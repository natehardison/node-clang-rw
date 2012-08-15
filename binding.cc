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
        clang_request->error = e.what();

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

    if (args.Length() < 2)
    {
      // throw some sort of exception
    }

    if (!args[0]->IsString())
    {
        return ThrowException(Exception::TypeError(
                    String::New("First arg should be a filename.")));
    }

    if (!(args[1]->IsString || args[1]->IsArray()))
    {
        // need either one function or an array of them
    }

    if (!args[2]->IsFunction())
    {
        // should be a callback
    }

    clang_request* req = malloc(sizeof(clang_request));
    req->filename = String:kkk
    req->callback = Persistent<Function>::New(callback);

    eio_custom(EIO_Rewrite, EIO_PRI_DEFAULT, EIO_AfterRewrite, clang_req);

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
