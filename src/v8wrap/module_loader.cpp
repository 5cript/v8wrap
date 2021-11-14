#include <v8wrap/module_loader.hpp>
#include <v8wrap/cast.hpp>

namespace v8wrap
{
//#####################################################################################################################
    v8::MaybeLocal<v8::Module> ModuleLoader::load(v8::Local<v8::Context> context, std::string const& path)
    {
        auto iter = synthetics_.find(path);
        if (iter != std::end(synthetics_))
            return loadSynthetic(context, path, iter->second);
        return v8::MaybeLocal<v8::Module>{};
    }
//---------------------------------------------------------------------------------------------------------------------
    v8::MaybeLocal<v8::Module> ModuleLoader::loadSynthetic
    (
        v8::Local<v8::Context> context, 
        std::string const& path, 
        ExportsContainer const& exports
    )
    {
        auto mod = *loadedModules_.insert(std::end(loadedModules_), std::make_shared <Module>(
            Module::SyntheticCreationParameters{
                .context = context,
                .fileName = path,
                .exports = exports,
                .onModuleLoad = [this](v8::Local<v8::Context> c, std::string const& p){
                    return load(c, p);
                }
            }
        ));
        mod->setup();
        return mod->getModule();
    }
//---------------------------------------------------------------------------------------------------------------------
    void ModuleLoader::addSynthetic(std::string const& path, ExportsContainer const& exports)
    {
        // TODO: 
        synthetics_[path] = exports;
    }
//---------------------------------------------------------------------------------------------------------------------
}