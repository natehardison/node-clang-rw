// Node.js includes
#include <node.h>
#include <v8.h>

// C++ includes
#include <exception>
#include <string>
#include <vector>

// Local includes
#include "rewriter.h"

using namespace node;
using namespace v8;

// EIO request struct to be passed to EIO functions (for async purposes)
typedef struct
{
    std::string filename;
    std::vector<std::string> functions;
    std::string error;
    Persistent<Function> callback;
}
rewrite_request;

int EIO_Rewrite(eio_req* req)
{
    HandleScope scope;

    // unpack our EIO request struct to get all of the info we need
    rewrite_request* rewrite_req = static_cast<rewrite_request*>(req);

    try
        rewrite(rewrite_req->filename, rewrite_req->functions);
    catch (exception& e)
        rewrite_request->error = std::string(e.what());

    return 0;
}

int EIO_AfterRewrite(eio_req* req)
{
    HandleScope scope;

    rewrite_request* rewrite_req = static_cast<clang_request*>(req);

    ev_unref(EV_DEFAULT_UC);

    // TODO: figure out callback signature
    rewrite_req->callback->Call(Context::GetCurrent()->Global(), 0);

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

    if (!(args[1]->IsString || args[1]->IsArray()))
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
    rewrite_request* rewrite_req = malloc(sizeof(rewrite_request));
    if (rewrite_req == NULL)
        return ThrowException(Exception::Error(
                    String::New("malloc in Remove failed.")));

    // pull out the filename arg and put a C++ string in our EIO request 
    Local<String> filename = args[0]->ToString();
    rewrite_req->filename = std::string(*(filename));

    // pull out the function arg(s) and put them in the EIO request's vector
    if (args[1]->IsString())
    {
        Local<String> function = args[1]->ToString();
        rewrite_req->functions.push_back(std::string(*(function)));
    }
    else
    {
        Local<Array> functions = Local<Array>::Cast(args[1]);
        for (int i = 0; i < functions->Length(); i++)
            rewrite_req->functions.push_back(std::string(*(functions[i]));
    }

    // pull out the callback and store a persistent copy in the EIO request
    Local<Function> callback = Local<Function>::Cast(args[2]); 
    rewrite_req->callback = Persistent<Function>::New(callback);

    // set up the EIO calls
    eio_custom(EIO_Rewrite, EIO_PRI_DEFAULT, EIO_AfterRewrite, rewrite_req);

    // retain a reference to this event thread so Node doesn't exit
    ev_ref(EV_DEFAULT_UC);

    return Undefined();
}

/*
Handle<Value> RemoveSync(const Arguments& args)
{
    HandleScope scope;

    try
        clang_rewrite(filename, functions);
    catch (exception& e)
        // do something with this error
        ;


    return Undefined();
}
*/

extern "C" void Initialize(v8::Handle<v8::Value> target)
{
    HandleScope scope;

    target->Set(String::NewSymbol("remove"), FunctionTemplate::New(Remove)->GetFunction());
    //target->Set(String::NewSymbol("remove_sync"), FunctionTemplate::New(RemoveSync)->GetFunction());
}

NODE_MODULE(clang, Initialize)
