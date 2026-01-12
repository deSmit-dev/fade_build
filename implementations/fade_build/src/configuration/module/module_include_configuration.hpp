#ifndef FADE_BUILD_CONFIGURATION_MODULE_INCLUDE_CONFIGURATION_HPP_
#define FADE_BUILD_CONFIGURATION_MODULE_INCLUDE_CONFIGURATION_HPP_

// Fade includes
#include "core/include/serialization/serialization.hpp"

// Fade Build includes
#include "configuration/version.hpp"

struct ModuleIncludeConfiguration
{
    Version version;
};

template <fade::InputArchiveType ArchiveType>
bool Serialize(ArchiveType& in_archive, ModuleIncludeConfiguration& out_module_include_config)
{
    ARCHIVE_PARAM(in_archive, out_module_include_config, version);
    return true;
}


#endif // FADE_BUILD_CONFIGURATION_MODULE_INTERACE_CONFIGURATION_HPP_