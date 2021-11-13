#pragma once

#include <v8.h>
#include <string>
#include <stdexcept>

namespace v8wrap
{
    class JavaScriptException : public std::runtime_error
    {
    public:
        JavaScriptException(std::string const& str);
    };

    void rethrowException(v8::Isolate* isolate, v8::TryCatch& jsException);
    void rethrowExceptionFallback(v8::Isolate* isolate, v8::TryCatch& jsException, std::string const& fallbackError);
}