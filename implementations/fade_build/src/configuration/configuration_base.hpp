#ifndef FADE_BUILD_CONFIGURATION_BASE_HPP_
#define FADE_BUILD_CONFIGURATION_BASE_HPP_

// Fade includes
#include "core/include/containers/dynamic_array.hpp"
#include "core/include/serialization/serialization.hpp"

// STL includes
#include <string>
#include <memory>
#include <filesystem>

/**
 * Configuration Base
 * 
 * 
 */
struct ConfigurationBase
{
    ConfigurationBase() = default;
    ConfigurationBase(const ConfigurationBase& in_rhs) = delete;
    ConfigurationBase(ConfigurationBase&& in_rhs) = default;

    ConfigurationBase& operator=(const ConfigurationBase& in_rhs) = delete;
    ConfigurationBase& operator=(ConfigurationBase&& in_rhs) = default;

    // The name of the module, should be all lowercase and underscores to represent spaces.
    std::string name;
    // The display name of the module
    std::string display_name;
    // The description of the module
    std::string description;
};

template <fade::InputArchiveType ArchiveType>
bool Serialize(ArchiveType& in_archive, ConfigurationBase& out_configuration_base)
{
    //ARCHIVE_PARAM(in_archive, out_configuration_base, name)
    //ARCHIVE_PARAM(in_archive, out_configuration_base, display_name)
    //ARCHIVE_PARAM(in_archive, out_configuration_base, description)
    return true;
}

#endif // FADE_BUILD_CONFIGURATION_BASE_HPP_