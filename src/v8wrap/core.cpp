#include <v8wrap/core.hpp>
#include <v8wrap/module.hpp>

#include <mutex>

namespace v8wrap
{
//#####################################################################################################################
    // FIXME: needs to be bound to isolate or context instance.
    std::mutex storeLock;
    std::unordered_multimap<uint32_t, std::weak_ptr<Module>> moduleStore;
//---------------------------------------------------------------------------------------------------------------------
    void unregisterModule(uint32_t hash, std::weak_ptr<Module> mod)
    {
        auto lockedMod = mod.lock();
        if (!lockedMod)
            return;

        std::scoped_lock guard{storeLock};

        auto range = moduleStore.equal_range(
            hash
        );
        for (auto it = range.first; it != range.second; ++it) 
        {
            const auto l = it->second.lock();
            if (!l)
            {
                //LOG() << "Erasing module with id " << it->first << ", it was not alive anymore [unregisterModule].\n";
                moduleStore.erase(it);
                continue;
            }
            if (l == lockedMod)
            {
                moduleStore.erase(it);
                break;
            }
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    void registerModule(uint32_t hash, std::weak_ptr<Module> mod)
    {
        std::scoped_lock guard{storeLock};
        moduleStore.emplace(hash, mod);
    }
//---------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<Module> findModule(v8::Local<v8::Module> mod)
    {
        std::scoped_lock guard{storeLock};
        auto range = moduleStore.equal_range(mod->GetIdentityHash());
        for (auto it = range.first; it != range.second; ++it) 
        {
            const auto l = it->second.lock();
            if (!l)
            {
                //LOG() << "Erasing module with id " << it->first << ", it was not alive anymore [findModule].\n";
                moduleStore.erase(it);
                continue;
            }
            if (l->getModule() == mod)
                return l;
        }
        return nullptr;
    }
//#####################################################################################################################
}