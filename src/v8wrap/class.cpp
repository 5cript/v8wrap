#include <v8wrap/class.hpp>

namespace v8wrap
{
    Class::operator v8::Local<v8::Value>()
    {
        return object_;
    }
}