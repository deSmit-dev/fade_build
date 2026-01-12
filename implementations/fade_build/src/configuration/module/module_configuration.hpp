#ifndef FADE_BUILD_CONFIGURATION_MODULE_CONFIGURATION_HPP_
#define FADE_BUILD_CONFIGURATION_MODULE_CONFIGURATION_HPP_

// Fade includes
#include "core/include/containers/dynamic_array.hpp"
#include "core/include/serialization/serialization.hpp"

// STL includes
#include <string>

/**
 * Fade Framework Module Configuration Entry
 * 
 * Contains information about this specific module and its implementation
 */
struct ModuleConfigurationEntry
{
    std::string module_name;

    std::string module_path;

    std::string implementation_name;

    std::string implementation_path;
};

template <fade::InputArchiveType ArchiveType>
bool Serialize(ArchiveType& in_archive, ModuleConfigurationEntry& out_module_configuration_entry)
{
    //ARCHIVE_PARAM(in_archive, out_module_configuration_entry, module_name)
    //ARCHIVE_PARAM(in_archive, out_module_configuration_entry, module_path);
    //ARCHIVE_PARAM(in_archive, out_module_configuration_entry, implementation_name);
    //ARCHIVE_PARAM(in_archive, out_module_configuration_entry, implementation_path);

    return true;
}

/**
 *  List of modules, their implementations and both paths
 */
struct ModuleConfiguration
{
    fade::DynamicArray<ModuleConfigurationEntry> configured_modules;

    bool IsValid() const
    {
        bool bValid = false;
        return bValid;
    }
};

template <fade::InputArchiveType ArchiveType>
bool Serialize(ArchiveType& in_archive, ModuleConfiguration& out_module_configuration)
{
    ARCHIVE_PARAM(in_archive, out_module_configuration, configured_modules)

    return true;
}

#endif // FADE_BUILD_CONFIGURATION_MODULE_CONFIGURATION_HPP_