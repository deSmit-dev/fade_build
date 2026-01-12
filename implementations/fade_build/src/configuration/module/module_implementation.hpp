#ifndef FADE_BUILD_CONFIGURATION_MODULE_IMPLEMENTATION_HPP_
#define FADE_BUILD_CONFIGURATION_MODULE_IMPLEMENTATION_HPP_

// Fade includes
#include "core/include/containers/dynamic_array.hpp"
#include "core/include/reflection/reflect_enum.hpp"
#include "core/include/type_definitions.hpp"

// Fade Build includes
#include "configuration/module/module_dependency.hpp"

// STL includes
#include <string>
#include <filesystem>

enum class Platform : fade::uint8
{
    kUnknown,
    kLinux,
    kWindows,
    kMac,
    kAndroid
};

template <>
Platform StringToEnum(const std::string& in_string)
{
    if (in_string == "linux") return Platform::kLinux;
    if (in_string == "windows") return Platform::kWindows;
    if (in_string == "mac") return Platform::kMac;
    if (in_string == "android") return Platform::kAndroid;

    return Platform::kUnknown;
}

template <>
std::string EnumToString(const Platform& in_platform)
{
    switch (in_platform)
    {
        case Platform::kLinux:
            return std::string("linux");
        case Platform::kWindows:
            return std::string("windows");
        case Platform::kMac:
            return std::string("mac");
        case Platform::kAndroid:
            return std::string("android");
        case Platform::kUnknown:
            break;
    }

    return std::string("unknown");
}

struct ModuleImplementationMetadata
{
    ModuleImplementationMetadata() = default;
    ModuleImplementationMetadata(ModuleImplementationMetadata&& in_other) = default;

    // Name of the implementation
    std::string name;

    // Description of the implementation
    std::string description;

    // What interface this implements
    std::string implements_interface;

    // Platform of this implementation
    Platform platform;

    // Dependencies of this implementation
    fade::DynamicArray<ModuleDependency> dependencies;
};

template <fade::InputArchiveType ArchiveType>
bool Serialize(ArchiveType& in_archive, ModuleImplementationMetadata& out_module_implementation)
{
    ARCHIVE_PARAM(in_archive, out_module_implementation, name)
    ARCHIVE_PARAM(in_archive, out_module_implementation, description)
    ARCHIVE_PARAM(in_archive, out_module_implementation, implements_interface)
    ARCHIVE_PARAM(in_archive, out_module_implementation, platform)
    ARCHIVE_PARAM(in_archive, out_module_implementation, dependencies);

    return true;
}

struct ModuleImplementation
{
    ModuleImplementationMetadata configuration;
    std::filesystem::path path;

};

#endif