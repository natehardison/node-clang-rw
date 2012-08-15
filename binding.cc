// Node.js includes
#include <node.h>
#include <v8.h>

// C++ includes
#include <exception>
#include <string>
#include <vector>

// Local includes
#include "clang-rewriter.h"

// EIO request struct to be passed to EIO functions (for async purposes)
typedef struct
{
    std::string filename;
    std::vector<std::string> functions;
    std::string error;
    Persistent<Function> callback;
}
clang_request;

/**
 * For making the asynchronous call to clang.
 */
int EIO_Rewrite(eio_req* req)
{
    HandleScope scope;

    // unpack our EIO request struct to get all of the info we need
    clang_request* clang_req = static_cast<clang_request*>(req);

    try
        clang_rewrite(clang_req->filename, clang_req->functions);
    catch (exception& e)
        clang_request->error = std::string(e.what());

    return 0;
}

int EIO_AfterRewrite(eio_req* req)
{
    HandleScope scope;

    clang_request* clang_req = static_cast<clang_request*>(req);

    ev_unref(EV_DEFAULT_UC);

    // TODO: figure out callback signature
    clang_req->callback->Call(Context::GetCurrent()->Global(), 0);

    clang_req->callback.Dispose();

    delete clang_req;

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
    clang_request* clang_req = malloc(sizeof(clang_request));
    if (clang_req == NULL)
        return ThrowException(Exception::Error(
                    String::New("malloc in Remove failed.")));

    // pull out the filename arg and put a C++ string in our EIO request 
    Local<String> filename = args[0]->ToString();
    clang_req->filename = std::string(*(filename));

    // pull out the function arg(s) and put them in the EIO request's vector
    if (args[1]->IsString())
    {
        Local<String> function = args[1]->ToString();
        clang_req->functions.push_back(std::string(*(function)));
    }
    else
    {
        Local<Array> functions = Local<Array>::Cast(args[1]);
        for (int i = 0; i < functions->Length(); i++)
            clang_req->functions.push_back(std::string(*(functions[i]));
    }

    // pull out the callback and store a persistent copy in the EIO request
    Local<Function> callback = Local<Function>::Cast(args[2]); 
    clang_req->callback = Persistent<Function>::New(callback);

    // set up the EIO calls
    eio_custom(EIO_Rewrite, EIO_PRI_DEFAULT, EIO_AfterRewrite, clang_req);

    // retain a reference to this event thread so Node doesn't exit
    ev_ref(EV_DEFAULT_UC);

    return Undefined();
}

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

extern "C" void Initialize(v8::Handle<v8::Value> target)
{
    HandleScope scope;

    target->Set(String::NewSymbol("remove"), FunctionTemplate::New(Remove)->GetFunction());
    target->Set(String::NewSymbol("remove_sync"), FunctionTemplate::New(RemoveSync)->GetFunction());
}

NODE_MODULE(clang, Initialize)
