#pragma once

#include <v8wrap/isolate.hpp>

#include <v8.h>

#include <memory>
#include <functional>
#include <string>

namespace v8wrap
{
    class Module : public std::enable_shared_from_this<Module>
    {
    public:
        struct CommonCreationParameters
        {
            using onModuleLoadFunction = std::function<v8::MaybeLocal<v8::Module>(
                v8::Local<v8::Context> context, 
                std::string const& path
            )>;
        };

        struct CreationParameters : public CommonCreationParameters
        {
            v8::Local<v8::Context> context;
            std::string fileName;
            std::string script;
            onModuleLoadFunction onModuleLoad;
        };

        struct SyntheticCreationParameters : public CommonCreationParameters
        {
            v8::Local<v8::Context> context;
            std::string fileName;
            v8::Local<v8::Value> exported;
            onModuleLoadFunction onModuleLoad;
        };

    public:
        Module(CreationParameters&& params);
        Module(SyntheticCreationParameters&& params);
        ~Module();
        Module(Module&&);
        Module& operator=(Module&&);

        void setup();
        v8::Local<v8::Value> evaluate();
        v8::Local<v8::Context>& context();
        v8::Local<v8::Module> getModule();
        v8::Local<v8::Value> getNamespace();
        std::vector<std::string> getModuleRequests() const;

    private:
        struct Implementation;
        std::unique_ptr <Implementation> impl_;
    };
}