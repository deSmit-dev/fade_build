#ifndef FADE_BUILD_CONFIGURATION_PROJECT_CONFIGURATION_HPP_
#define FADE_BUILD_CONFIGURATION_PROJECT_CONFIGURATION_HPP_

// Fade includes
#include "core/include/serialization/serialization.hpp"

// Fade build includes
#include "configuration/version.hpp"

// STL includes
#include <string>


struct ProjectConfiguration
{
    // The name of the project
    std::string name;

    // The description of the project
    std::string description;

    // The module used that contains the entry point
    std::string entry_module;

    // The version of the project
    Version version;

    bool operator==(const ProjectConfiguration& other) const
    {
        return name == other.name && version == other.version;
    }
};

template <fade::InputArchiveType ArchiveType>
bool Serialize(ArchiveType& in_archive, ProjectConfiguration& out_project_configuration)
{
    ARCHIVE_PARAM(in_archive, out_project_configuration, name)
    ARCHIVE_PARAM(in_archive, out_project_configuration, description)
    ARCHIVE_PARAM(in_archive, out_project_configuration, entry_module)
    ARCHIVE_PARAM(in_archive, out_project_configuration, version)

    return true;
}

#endif // FADE_BUILD_CONFIGURATION_PROJECT_CONFIGURATION_HPP_