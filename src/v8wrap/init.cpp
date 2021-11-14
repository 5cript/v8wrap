#include <v8wrap/init.hpp>

#include <libplatform/libplatform.h>
#include <v8.h>

namespace v8wrap
{
//#####################################################################################################################
    struct GlobalInit::Implementation
    {
        std::unique_ptr<v8::Platform> platform;

        Implementation()
            : platform{v8::platform::NewDefaultPlatform()}
        {}
    };
//#####################################################################################################################
    GlobalInit::GlobalInit(std::filesystem::path const& selfPath)
        : impl_{std::make_unique<Implementation>()}
    {
        v8::V8::InitializeICUDefaultLocation(selfPath.string().c_str());
        v8::V8::InitializeExternalStartupData(selfPath.string().c_str());
        v8::V8::InitializePlatform(impl_->platform.get());
        v8::V8::Initialize();
    }
//---------------------------------------------------------------------------------------------------------------------    
    GlobalInit::~GlobalInit()
    {
        v8::V8::Dispose();
        v8::V8::ShutdownPlatform();
    }
//#####################################################################################################################
}