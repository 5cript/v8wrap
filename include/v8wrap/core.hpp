#pragma once

#include <v8.h>

#include <unordered_map>
#include <cstdint>
#include <memory>

namespace v8wrap
{
    class Module;

    //extern std::unordered_multimap<uint32_t, Module*> moduleStore;
    void registerModule(uint32_t hash, std::weak_ptr<Module> mod);
    void unregisterModule(uint32_t hash, std::weak_ptr<Module> mod);
    std::shared_ptr<Module> findModule(v8::Local<v8::Module> mod);
}