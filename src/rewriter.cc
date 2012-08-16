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
    std::string filename;
    std::set<std::string> functions;
    std::string error;
    Persistent<Function> callback;
}
rewrite_request;

static int EIO_Rewrite(eio_req* req)
{
    std::cerr << "Here we are!" << std::endl;

    // unpack our EIO request struct to get all of the info we need
    rewrite_request* rewrite_req = static_cast<rewrite_request*>(req->data);

    //try
        rewrite(rewrite_req->filename, rewrite_req->functions);
    //catch (const char* err)
        //rewrite_req->error = strdup(err);

    return 0;
}

int EIO_AfterRewrite(eio_req* req)
{
    HandleScope scope;

    rewrite_request* rewrite_req = static_cast<rewrite_request*>(req->data);

    ev_unref(EV_DEFAULT_UC);

    // TODO: figure out if this is appropriate for the callback
    Handle<Value> argv[1];
    if (!rewrite_req->error.empty())
        argv[0] = String::New(rewrite_req->error.c_str());
    else
        argv[0] = Undefined();

    TryCatch try_catch;

    rewrite_req->callback->Call(Context::GetCurrent()->Global(), 1, argv);

    if (try_catch.HasCaught())
        FatalException(try_catch);

    // clean up
    rewrite_req->callback.Dispose();
    delete rewrite_req;

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
    rewrite_request* rewrite_req = new rewrite_request();

    // pull out the filename arg and put a C++ string in our EIO request 
    String::Utf8Value filename(args[0]);
    rewrite_req->filename = std::string(*filename);

    std::cerr << "Filename: " << rewrite_req->filename << std::endl;

    // pull out the function arg(s) and put them in the EIO request's vector
    if (args[1]->IsString())
    {
        String::Utf8Value function(args[1]);
        rewrite_req->functions.insert(std::string(*function));
    }
    else
    {
        Local<Array> functions = Local<Array>::Cast(args[1]);
        for (unsigned int i = 0; i < functions->Length(); i++)
        {
            String::Utf8Value function(functions->Get(i)->ToString());
            rewrite_req->functions.insert(std::string(*function));
            std::cout << "Function: " << *function << std::endl;
        }
    }

    // pull out the callback and store a persistent copy in the EIO request
    Local<Function> callback = Local<Function>::Cast(args[2]); 
    rewrite_req->callback = Persistent<Function>::New(callback);

    // set up the EIO calls
    eio_custom(EIO_Rewrite, EIO_PRI_MAX, EIO_AfterRewrite, rewrite_req);

    // retain a reference to this event thread so Node doesn't exit
    ev_ref(EV_DEFAULT_UC);

    return Undefined();
}

void Initialize(Handle<Object> target)
{
    HandleScope scope;

    target->Set(String::NewSymbol("remove"), FunctionTemplate::New(Remove)->GetFunction());
}

NODE_MODULE(rewriter, Initialize)
