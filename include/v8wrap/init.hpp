#pragma once

#include <filesystem>
#include <memory>

namespace v8wrap
{
    class GlobalInit
    {
    public:
        GlobalInit(std::filesystem::path const& selfPath);
        ~GlobalInit();

    private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}