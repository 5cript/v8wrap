#pragma once

#include <v8wrap/exception.hpp>
#include <v8wrap/function.hpp>
#include <v8.h>

namespace v8wrap
{
    /**
     * A wrapper for a JavaScript class which instantiates it and makes it accessible.
     */
    class Class
    {
    public:
        /**
         * If you have a class A, pass A here.
         */
        template <typename... Forwards>
        Class(v8::Local<v8::Context> context, v8::Local<v8::Value> constructor, Forwards&&... fwds)
            : context_{context}
        {
            v8::EscapableHandleScope scope(context_->GetIsolate());
            if (!constructor->IsFunction())
                throw std::runtime_error("Passed argument needs to be a constructor funciton.");

            v8::TryCatch exc(context_->GetIsolate());
            auto maybeObj = constructor->ToObject(context_);
            v8::Local<v8::Object> obj;
            if (maybeObj.IsEmpty() || !maybeObj.ToLocal(&obj))
                rethrowExceptionFallback(context_->GetIsolate(), exc, "Cannot convert constructor to callable.");

            auto maybeClass = forwardCall(context_, [&obj, this](auto params){
                return obj->CallAsConstructor(context_, sizeof...(Forwards), params);
            }, std::forward<Forwards>(fwds)...);   
            rethrowException(context_->GetIsolate(), exc);  

            v8::Local<v8::Value> val;
            if (!maybeClass.ToLocal(&val))
                rethrowExceptionFallback(context_->GetIsolate(), exc, "Construction failed.");
            
            auto maybeObject = val->ToObject(context_);
            if (!maybeObject.ToLocal(&object_))
                rethrowExceptionFallback(context_->GetIsolate(), exc, "Constructor did not produce an object.");
            object_ = scope.Escape(object_);
        }

        template <typename... Forwards>
        v8::Local<v8::Value> callMember(std::string const& name, Forwards&&... fwds)
        {
            using namespace std::string_literals;
            v8::EscapableHandleScope scope(context_->GetIsolate());

            auto funcName = v8cast<std::string>(context_, name);
            v8::TryCatch exc(context_->GetIsolate());
            auto hasMember = object_->Has(context_, funcName);
            if (hasMember.IsNothing() || !hasMember.ToChecked())
                throw std::invalid_argument("Class does not have member with name '"s + name + "'.");

            v8::Local<v8::Value> funcAsValue;
            if (auto maybe = object_->Get(context_, funcName); !maybe.ToLocal(&funcAsValue))
                rethrowExceptionFallback(context_->GetIsolate(), exc, "Cannot get member.");
            v8::Local<v8::Object> func;
            if (auto maybe = funcAsValue->ToObject(context_); !maybe.ToLocal(&func))
                rethrowExceptionFallback(context_->GetIsolate(), exc, "Could not convert member to object.");

            v8::Local<v8::Value> result;
            if (auto maybe = forwardCall(context_, [&func, this](auto params){
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