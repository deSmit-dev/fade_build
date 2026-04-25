#ifndef FADE_BUILD_CONFIGURATION_MODULE_IMPLEMENTATION_HPP_
#define FADE_BUILD_CONFIGURATION_MODULE_IMPLEMENTATION_HPP_

// Fade includes
#include "core/include/containers/dynamic_array.hpp"
#include "core/include/type_definitions.hpp"

// Fade Build includes
#include "configuration/configuration_base.hpp"
#include "configuration/module/module_dependency.hpp"
#include "fade_build_types.hpp"

// STL includes
#include <string>
#include <filesystem>

struct ModuleImplementationConfiguration : public ConfigurationBase
{
    ModuleImplementationConfiguration() = default;
    ModuleImplementationConfiguration(const ModuleImplementationConfiguration& in_other) = delete;
    ModuleImplementationConfiguration(ModuleImplementationConfiguration&& in_other) = default;

    ModuleImplementationConfiguration& operator=(const ModuleImplementationConfiguration& in_other) = delete;
    ModuleImplementationConfiguration& operator=(ModuleImplementationConfiguration&& in_other) = default;

    // What interface this implements
    std::string implements_interface;

    // Platform of this implementation
    Platform platform;

    // Dependencies of this implementation
    fade::DynamicArray<ModuleDependency> dependencies;
};

template <fade::InputArchiveType ArchiveType, typename T>
bool Serialize(ArchiveType& in_archive, ModuleImplementationConfiguration& out_module_implementation)
{
    Serialize(in_archive, static_cast<ConfigurationBase&>(out_module_implementation));
    /*ARCHIVE_PARAM(in_archive, out_module_implementation, implements_interface)
    ARCHIVE_PARAM(in_archive, out_module_implementation, platform)
    ARCHIVE_PARAM(in_archive, out_module_implementation, dependencies);*/

    return true;
}

struct ModuleImplementation
{
    ModuleImplementationConfiguration configuration;

    std::string filename;

    std::filesystem::path path;
};

#endif