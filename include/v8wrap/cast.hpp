#pragma once

#include <v8wrap/exception.hpp>

#include <v8.h>

#include <stdexcept>
#include <utility>
#include <limits>

// v8cast<std::string>(local)

namespace v8wrap
{
    template <typename T, typename Enable = void>
    struct Caster
    {
    };

    template <>
    struct Caster<std::string, void>
    {
        static std::string cast(v8::Local<v8::Context> ctx, v8::Local<v8::Value> value)
        {
            if (value.IsEmpty() || !value->IsString())
                throw std::invalid_argument("Value is not a string.");

            return cast(ctx, v8::String::Utf8Value{ctx->GetIsolate(), value});
        }
        static std::string cast(v8::Local<v8::Context> ctx, v8::Local<v8::String> value)
        {
            return cast(ctx, v8::String::Utf8Value{ctx->GetIsolate(), value});
        }
        static std::string cast(v8::Local<v8::Context>, v8::String::Utf8Value value)
        {
            return std::string(*value);
        }
        static v8::Local<v8::String> cast(v8::Local<v8::Context> ctx, std::string const& str)
        {
            return cast(ctx, str.c_str());
        }
        static v8::Local<v8::String> cast(v8::Local<v8::Context> ctx, std::string_view view)
        {
            // Do note that string_view ist not necessarily null terminated so copying it to a terminated buffer
            // is unfortunately necessary.
            return cast(ctx, std::string{view});
        }
        static v8::Local<v8::String> cast(v8::Local<v8::Context> ctx, char const* cstr)
        {
            return v8::String::NewFromUtf8(ctx->GetIsolate(), cstr).ToLocalChecked();
        }
    };
    template <>
    struct Caster<char const*, void> : public Caster<std::string, void>
    {};

    template <>
    struct Caster <bool, void>
    {
        static bool cast(v8::Local<v8::Context> ctx, v8::Local<v8::Value> value)
        {
            if (value.IsEmpty() || !value->IsBoolean())
                throw std::invalid_argument("Value is not a boolean.");

            return value->BooleanValue(ctx->GetIsolate());
        }
        static v8::Local<v8::Value> cast(v8::Local<v8::Context> ctx, bool value)
        {
            return v8::Boolean::New(ctx->GetIsolate(), value);
        }
    };

    template <typename T>
    struct Caster <T, std::enable_if_t<std::is_integral_v<T>>>
    {
        constexpr static auto size = CHAR_BIT * sizeof(T);
        constexpr static auto isSigned = std::is_signed_v<T>;

        static T cast(v8::Local<v8::Context> ctx, v8::Local<v8::Value> value)
        {
            if (value.IsEmpty() || !value->IsNumber())
                throw std::invalid_argument("Value is not a number.");
            
            if (size <= 32)
            {
                if (isSigned)
                    return static_cast<T>(value->Int32Value(ctx).FromJust());
                else
                    return static_cast<T>(value->Uint32Value(ctx).FromJust());
            }
            else
                return static_cast<T>(value->IntegerValue(ctx).FromJust());
        }

        static v8::Local<v8::Value> cast(v8::Local<v8::Context> ctx, T value)
        {
            if (size <= 32)
            {
                if (isSigned)
                    return v8::Integer::New(ctx->GetIsolate(), static_cast<int32_t>(value));
                else
                    return v8::Integer::NewFromUnsigned(ctx->GetIsolate(), static_cast<uint32_t>(value));
            }
            else
            {
                if (isSigned)
                    return v8::BigInt::New(ctx->GetIsolate(), value);
                else
                    return v8::BigInt::NewFromUnsigned(ctx->GetIsolate(), value);
            }
        }
    };    

    template <typename T>
    struct Caster <T, std::enable_if_t<std::is_floating_point_v<T>>>
    {
        static T cast(v8::Local<v8::Context> ctx, v8::Local<v8::Value> value)
        {
            if (value.IsEmpty() || !value->IsNumber())
                throw std::invalid_argument("Value is not a number");
            
            return static_cast<T>(value->NumberValue(ctx).FromJust());
        }

        static v8::Local<v8::Number> cast(v8::Local<v8::Context> ctx, T value)
        {
            return v8::Number::New(ctx->GetIsolate(), value);
        }
    };

    /**
     *  Use to convert C++ values to v8 values and back.
     */
    template <typename Subject, typename FromT>
    auto v8cast(v8::Local<v8::Context> ctx, FromT&& value)
    {
        return Caster<Subject>::cast(ctx, std::forward<FromT>(value));
    }
}