#pragma once

#include <v8wrap/cast.hpp>
#include <v8.h>

namespace v8wrap
{
    inline std::string typeOf(v8::Local<v8::Context> context, v8::Local<v8::Value> const& value)
    {
        return v8cast<std::string>(context, value->TypeOf(context->GetIsolate()));
    }
}