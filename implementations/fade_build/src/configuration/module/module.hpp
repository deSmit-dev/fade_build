#ifndef FADE_BUILD_CONFIGURATION_MODULE_MODULE_HPP_
#define FADE_BUILD_CONFIGURATION_MODULE_MODULE_HPP_

// Fade includes
#include "core/include/containers/dynamic_array.hpp"
#include "core/include/serialization/serialization.hpp"
#include "core/include/serialization/input_archive.hpp"

// Fade Build includes
#include "configuration/module/module_dependency.hpp"
#include "configuration/module/module_implementation.hpp"
#include "configuration/module/module_include_configuration.hpp"
#include "configuration/module/module_interface_configuration.hpp"

// STL includes
#include <string>
#include <memory>
#include <filesystem>

/**
 * Module Configuration
 * 
 * Some basic metadata to describe the module.
 */
struct ModuleConfiguration
{
    ModuleConfiguration() = default;
    ModuleConfiguration(const ModuleConfiguration& in_rhs) = delete;
    ModuleConfiguration(ModuleConfiguration&& in_rhs) = default;

    ModuleConfiguration& operator=(const ModuleConfiguration& in_rhs) = delete;
    ModuleConfiguration& operator=(ModuleConfiguration&& in_rhs) = default;

    // The name of the module
    std::string name;
    // The description of the module
    std::string description;
    // The optional interface metadata of this module
    std::unique_ptr<Version> interface_version;
    // The optional source metadata of this module
    std::unique_ptr<Version> include_version;
    // Dependencies for this module
    fade::DynamicArray<ModuleDependency> dependencies;

    // Whether this module implements the main function
    bool implements_main = false;

    /**
     * Whether this module has platform specific implementations
     * This means, if the user doesn't specify a specific implementation (that corresponds with the target platform), the system will automatically find the right one
     */
    bool has_platform_implementations = false;
};

template <fade::InputArchiveType ArchiveType>
bool Serialize(ArchiveType& in_archive, ModuleConfiguration& out_module_metadata)
{
    ARCHIVE_PARAM(in_archive, out_module_metadata, name)
    ARCHIVE_PARAM(in_archive, out_module_metadata, description)
    ARCHIVE_PARAM(in_archive, out_module_metadata, interface_version)
    ARCHIVE_PARAM(in_archive, out_module_metadata, include_version)
    ARCHIVE_PARAM(in_archive, out_module_metadata, dependencies)
    ARCHIVE_PARAM(in_archive, out_module_metadata, implements_main)
    ARCHIVE_PARAM(in_archive, out_module_metadata, has_platform_implementations)
    return true;
}

/**
 * Fade Framework Module
 * 
 * A module is the main building block of the Fade Framework.
 * A module may or may not contain an interface, which must be implemented by one or more implementations.
 * A module may also contain only source and/or include files, this is especially useful for modules that contain third party code since they usually come in this format.
 * 
 * The implementations mentioned previously may be provided by the framework, or implemented by the end user. But must adhere to the provided interface.
 */
struct Module
{
    Module() = default;
    Module(const Module& in_rhs) = delete;
    Module(Module&& in_rhs) = default;

    Module& operator=(const Module& in_rhs) = delete;
    Module& operator=(Module&& in_rhs) = default;

    /**
     * Name of the module file
     * 
     * The string used to match implementations to modules.
     */
    std::string filename;

    /**
     * Path to this module
     */
    std::filesystem::path module_path;

    /**
     * Module configuration
     */
    ModuleConfiguration configuration;

    /**
     * Implementations for this module
     */
    fade::DynamicArray<ModuleImplementation> implementations;
};

#endif // FADE_BUILD_CONFIGURATION_MODULE_MODULE_HPP_