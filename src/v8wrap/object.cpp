#include <v8wrap/object.hpp>

namespace v8wrap
{
    Class::operator v8::Local<v8::Value>()
    {
        return object_;
    }
}