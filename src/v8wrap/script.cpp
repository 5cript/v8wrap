#include <v8wrap/script.hpp>
#include <v8wrap/exception.hpp>
#include <v8wrap/cast.hpp>

namespace v8wrap
{
//#####################################################################################################################
    Script::Script(CreationParameters&& params)
        : scriptContext{v8::Context::New(params.isolate)}
        , contextScope{scriptContext}
        , origin{
            v8::String::NewFromUtf8(params.isolate, params.fileName.c_str()).ToLocalChecked(),      // specifier
            v8::Integer::New(params.isolate, 0),             // line offset
            v8::Integer::New(params.isolate, 0),             // column offset
            v8::False(params.isolate),                       // is cross origin
            v8::Local<v8::Integer>(),                     // script id
            v8::Local<v8::Value>(),                       // source map URL
            v8::False(params.isolate),                       // is opaque
            v8::False(params.isolate),                       // is WASM
            v8::True(params.isolate)                         // is ES6 module
        }
        , source{
            v8::String::NewFromUtf8(params.isolate, params.script.c_str(), v8::NewStringType::kNormal).ToLocalChecked(), 
            origin
        }
        , script{[this, &params]() {
            v8::EscapableHandleScope scope{scriptContext->GetIsolate()};
            v8::TryCatch exc(scriptContext->GetIsolate());
            v8::MaybeLocal<v8::Script> scr = v8::Script::Compile(scriptContext, v8cast<std::string>(scriptContext, params.script));
            if (scr.IsEmpty())
            {
                if (exc.HasCaught())
                    rethrowException(scriptContext->GetIsolate(), exc);
                else
                    throw std::runtime_error("Could not compile script.");
            }
            return scope.Escape(scr.ToLocalChecked());
        }()}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    Script::~Script() = default;
//---------------------------------------------------------------------------------------------------------------------
    v8::Local<v8::Value> Script::run()
    {
        v8::EscapableHandleScope scope{scriptContext->GetIsolate()};
        v8::TryCatch exc(scriptContext->GetIsolate());
        auto maybeRan = script->Run(scriptContext);
        if (maybeRan.IsEmpty())
        {
            if (exc.HasCaught())
                rethrowException(scriptContext->GetIsolate(), exc);
            else
                throw std::runtime_error("Unknown error in script.");
        }
        return scope.Escape(maybeRan.ToLocalChecked());
    }
//---------------------------------------------------------------------------------------------------------------------
    v8::Isolate* Script::isolate()
    {
        return scriptContext->GetIsolate();
    }
//---------------------------------------------------------------------------------------------------------------------
    v8::Local<v8::Context>& Script::context()
    {
        return scriptContext;
    }
//#####################################################################################################################
}