// Node.js includes
#include <node.h>
#include <v8.h>

// C++ includes
#include <exception>
#include <iostream>
#include <set>
#include <string>

// Local includes
#include "clang_rewriter.h"

using namespace node;
using namespace v8;

// EIO request struct to be passed to EIO functions (for async purposes)
typedef struct
{
    char* filename;
    char* function;
    char* error;
    Persistent<Function> callback;
}
rewrite_request;

int EIO_Rewrite(eio_req* req)
{
    HandleScope scope;

    // unpack our EIO request struct to get all of the info we need
    rewrite_request* rewrite_req = (rewrite_request*)req->data;

    std::cout << "Here we are!" << std::endl;
    try
        rewrite(rewrite_req->filename, rewrite_req->function);
    catch (const char* err)
        rewrite_req->error = strdup(err);

    return 0;
}

int EIO_AfterRewrite(eio_req* req)
{
    HandleScope scope;

    rewrite_request* rewrite_req = (rewrite_request*)req->data;

    ev_unref(EV_DEFAULT_UC);

    // TODO: figure out if this is appropriate for the callback
    Local<Value> argv[1];
    if (rewrite_req->error)
        argv[0] = ErrorException(rewrite_req->error);
    else
        argv[0] = Undefined();

    TryCatch try_catch;

    rewrite_req->callback->Call(Context::GetCurrent()->Global(), 1, argv);

    if (try_catch.HasCaught())
        FatalException(try_catch);

    // clean up
    free(rewrite_req->filename);
    free(rewrite_req->function);
    rewrite_req->callback.Dispose();
    if (rewrite_req->error)
        free(rewrite_req->error);

    free(rewrite_req);

    return 0;
}

Handle<Value> Remove(const Arguments& args)
{
    HandleScope scope;

    // error checking! It sucks to to it this way...
    if (args.Length() < 3)
    {
        return ThrowException(Exception::Error(
                    String::New("Usage: remove(filename, function[s], callback)")));
    }

    if (!args[0]->IsString())
    {
        return ThrowException(Exception::TypeError(
                    String::New("First arg should be a filename.")));
    }

    if (!(args[1]->IsString() || args[1]->IsArray()))
    {
        return ThrowException(Exception::TypeError(
                    String::New("Second arg should be a string or an array.")));
    }

    if (!args[2]->IsFunction())
    {
        return ThrowException(Exception::TypeError(
                    String::New("Third arg should be a callback function.")));
    }

    // build up our EIO request to be passed to our async EIO_ functions
    rewrite_request* rewrite_req = (rewrite_request*)malloc(sizeof(rewrite_request));

    // pull out the filename arg and put a C++ string in our EIO request 
    String::Utf8Value filename(args[0]);
    rewrite_req->filename = strdup(*filename);

    std::cerr << "Filename: " << rewrite_req->filename << std::endl;

    // pull out the function arg(s) and put them in the EIO request's vector
    if (args[1]->IsString())
    {
        String::Utf8Value function(args[1]);
        rewrite_req->functions = strdup(*function);
    }
    /*else
    {
        Local<Array> functions = Local<Array>::Cast(args[1]);
        for (unsigned int i = 0; i < functions->Length(); i++)
        {
            String::Utf8Value function(functions->Get(i)->ToString());
            rewrite_req->functions.insert(std::string(*function));
            std::cout << "Function: " << *function << std::endl;
        }
    }*/

    // pull out the callback and store a persistent copy in the EIO request
    Local<Function> callback = Local<Function>::Cast(args[2]); 
    rewrite_req->callback = Persistent<Function>::New(callback);

    // initialize our error string to NULL
    rewrite_req->error = NULL;

    // set up the EIO calls
    eio_custom(EIO_Rewrite, EIO_PRI_DEFAULT, EIO_AfterRewrite, rewrite_req);

    // retain a reference to this event thread so Node doesn't exit
    uv_ref(EV_DEFAULT_UC);

    return Undefined();
}

void Initialize(Handle<Object> target)
{
    HandleScope scope;

    target->Set(String::NewSymbol("remove"), FunctionTemplate::New(Remove)->GetFunction());
}

NODE_MODULE(rewriter, Initialize)
