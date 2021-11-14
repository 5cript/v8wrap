#pragma once

#include <v8wrap/cast.hpp>
#include <v8.h>

#include <tuple>
#include <type_traits>
#include <tuple>

namespace v8wrap
{
    template <typename... Forwards>
    v8::Local<v8::Value> argumentsAsArray(v8::Local<v8::Context> context, Forwards&&... forwards)
    {
        v8::Local<v8::Array> params = v8::Array::New(context->GetIsolate(), sizeof...(forwards));
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (void)std::initializer_list<v8::Maybe<bool>>{params->Set(
                context, 
                Is, 
                v8cast<std::decay_t<std::tuple_element_t<Is, std::tuple<Forwards...>>>>(context, std::forward<Forwards>(forwards))
            )...};
        }(std::make_index_sequence<sizeof...(forwards)>{});
        return params;
    }

    template <typename FunctionCall, typename... Forwards>
    auto forwardCall(v8::Local<v8::Context> context, FunctionCall const& call, Forwards&&... forwards)
    {
        if constexpr (sizeof...(forwards) != 0) 
        {
            return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                v8::Local<v8::Value> args[sizeof...(Forwards)] = {
                    v8cast<std::decay_t<std::tuple_element_t<Is, std::tuple<Forwards...>>>>(context, std::forward<Forwards>(forwards))...
                };
                return call(sizeof...(forwards), args);
            }(std::make_index_sequence<sizeof...(forwards)>{});
        }
        else
        {
            return call(0, nullptr);
        }
    }
}