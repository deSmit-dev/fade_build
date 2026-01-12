#ifndef FADE_BUILD_CONFIGURATION_MODULE_DEPENDENCY_HPP_
#define FADE_BUILD_CONFIGURATION_MODULE_DEPENDENCY_HPP_

// Fade includes
#include "core/include/serialization/serialization.hpp"

// Fade Build includes
#include "configuration/version.hpp"

// STL includes
#include <string>

struct ModuleDependency
{
    ModuleDependency() = default;
    ModuleDependency(ModuleDependency&& in_other) = default;

    // Name of the module
    std::string name;

    // Optional specific implementation of the interface
    std::string implementation;

    // Version of the module's includes
    std::unique_ptr<Version> include_version;

    // Version of the module's interface
    std::unique_ptr<Version> interface_version;
};

template <fade::InputArchiveType ArchiveType>
bool Serialize(ArchiveType& in_archive, ModuleDependency& out_module_dependency)
{
    ARCHIVE_PARAM(in_archive, out_module_dependency, name)
    ARCHIVE_PARAM(in_archive, out_module_dependency, include_version)
    ARCHIVE_PARAM(in_archive, out_module_dependency, interface_version)

    return true;
}

#endif // FADE_BUILD_CONFIGURATION_MODULE_DEPENDENCY_HPP_
