#pragma once

#include <v8wrap/module_export.hpp>
#include <v8wrap/module.hpp>
#include <v8.h>

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

namespace v8wrap
{
    class ModuleLoader
    {
    public:
        v8::MaybeLocal<v8::Module> load(v8::Local<v8::Context> context, std::string const& path);
        void addSynthetic(std::string const& path, ExportsContainer const& exports);

    private:
        v8::MaybeLocal<v8::Module> loadSynthetic
        (
            v8::Local<v8::Context> context, 
            std::string const& path, 
            ExportsContainer const& exports
        );
    
    private:
        std::vector<std::shared_ptr <Module>> loadedModules_;
        std::unordered_map<std::string, ExportsContainer> synthetics_;
    };
}