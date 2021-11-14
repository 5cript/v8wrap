#include <v8.h>
#include <variant>
#include <unordered_map>

namespace v8wrap
{    
    /*using ExportType = std::variant<
        v8::Local<v8::Value>,
        v8::Local<v8::ObjectTemplate>,
        v8::Local<v8::FunctionTemplate>
    >;
    */
   using ExportType = v8::Local<v8::Value>;
    using ExportsContainer = std::unordered_map <std::string, ExportType>;
}