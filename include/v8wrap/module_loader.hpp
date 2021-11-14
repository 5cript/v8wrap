#pragma once

#include <v8wrap/module.hpp>
#include <v8.h>

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

namespace v8wrap
{
    class ModuleLoader
    {
    public:
        v8::MaybeLocal<v8::Module> load(v8::Local<v8::Context> context, std::string const& path);
        void addSynthetic(std::string const& path, v8::Local<v8::Value>);

    private:
        v8::MaybeLocal<v8::Module> loadSynthetic(v8::Local<v8::Context> context, std::string const& path, v8::Local<v8::Value> value);
    
    private:
        std::vector<std::shared_ptr <Module>> loadedModules_;
        std::unordered_map<std::string, v8::Local<v8::Value>> synthetics_;
    };
}