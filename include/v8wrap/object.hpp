#pragma once

#include <v8wrap/exception.hpp>
#include <v8wrap/function.hpp>
#include <v8wrap/value.hpp>
#include <v8.h>

namespace v8wrap
{
    /**
     * A wrapper for a JavaScript objects and classes.
     */
    class Object
    {
    public:
        template <typename... Forwards>
        static std::unique_ptr<Object> instantiateClass(v8::Local<v8::Context> context, v8::Local<v8::Value> constructor, Forwards&&... fwds)
        {
            using namespace std::string_literals;

            v8::EscapableHandleScope scope(context->GetIsolate());
            if (!constructor->IsFunction())
                throw std::runtime_error("Passed argument needs to be a constructor function in class instantiation; the type is: "s + typeOf(context, constructor));

            v8::TryCatch exc(context->GetIsolate());
            auto maybeObj = constructor->ToObject(context);
            v8::Local<v8::Object> obj;
            if (maybeObj.IsEmpty() || !maybeObj.ToLocal(&obj))
                rethrowExceptionFallback(context->GetIsolate(), exc, "Cannot convert constructor to callable.");

            auto maybeClass = forwardCall(context, [&obj, &context](auto, auto params){
                return obj->CallAsConstructor(context, sizeof...(Forwards), params);
            }, std::forward<Forwards>(fwds)...);   
            rethrowException(context->GetIsolate(), exc);  

            v8::Local<v8::Value> val;
            if (!maybeClass.ToLocal(&val))
                rethrowExceptionFallback(context->GetIsolate(), exc, "Construction failed.");
            
            v8::Local<v8::Object> object;
            auto maybeObject = val->ToObject(context);
            if (!maybeObject.ToLocal(&object))
                rethrowExceptionFallback(context->GetIsolate(), exc, "Constructor did not produce an object.");

            return std::make_unique<Object>(context, scope.Escape(object));
        }

    public:
        Object(v8::Local<v8::Context> context, v8::Local<v8::Object> obj)
            : context_{context}
            , object_{obj}
        {
        }

        template <typename... Forwards>
        v8::Local<v8::Value> call(std::string const& name, Forwards&&... fwds)
        {
            using namespace std::string_literals;
            v8::EscapableHandleScope scope(context_->GetIsolate());

            auto funcName = v8cast<std::string>(context_, name);
            v8::TryCatch exc(context_->GetIsolate());
            auto hasMember = object_->Has(context_, funcName);
            if (hasMember.IsNothing() || !hasMember.ToChecked())
                throw std::invalid_argument("Object does not have member with name '"s + name + "'.");

            v8::Local<v8::Value> funcAsValue;
            if (auto maybe = object_->Get(context_, funcName); !maybe.ToLocal(&funcAsValue))
                rethrowExceptionFallback(context_->GetIsolate(), exc, "Cannot get member.");
            v8::Local<v8::Object> func;
            if (auto maybe = funcAsValue->ToObject(context_); !maybe.ToLocal(&func))
                rethrowExceptionFallback(context_->GetIsolate(), exc, "Could not convert member to object.");

            v8::Local<v8::Value> result;
            if (auto maybe = forwardCall(context_, [&func, this](auto, auto params){
                return func->CallAsFunction(context_, object_, sizeof...(Forwards), params);
            }, std::forward<Forwards>(fwds)...); !maybe.ToLocal(&result))
            {
                rethrowExceptionFallback(context_->GetIsolate(), exc, "There was an error calling the function.");
            }
            return scope.Escape(result);
        }

        operator v8::Local<v8::Value>();

    private: 
        v8::Local<v8::Context> context_;
        v8::Local<v8::Object> object_;
    };
}