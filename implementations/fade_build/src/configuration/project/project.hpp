#ifndef FADE_BUILD_CONFIGURATION_PROJECT_CONFIGURATION_HPP_
#define FADE_BUILD_CONFIGURATION_PROJECT_CONFIGURATION_HPP_

// Fade includes
#include "core/include/serialization/serialization.hpp"

// Fade build includes
#include "configuration/version.hpp"
#include "configuration/module/module_implementation_configuration.hpp"

// STL includes
#include <string>
#include <filesystem>

struct ProjectConfiguration
{
public:
    // The name of the project
    std::string name;

    // The description of the project
    std::string description;

    // The module used that contains the entry point
    std::string entry_module;

    // The version of the project
    Version version;

    // Chosen implementations for the modules used by the project
    fade::DynamicArray<ModuleImplementationPair> chosen_implementations;

    // The module implementation descriptions for this project
    fade::DynamicArray<ImplementationDescription> implementation_overrides;

public:
    bool HasImplementationDescriptions()
    {
        return chosen_implementations.size() > 0 || implementation_overrides.size() > 0;
    }


    /* Operators */
    bool operator==(const ProjectConfiguration& other) const
    {
        return name == other.name && version == other.version;
    }
};

template <fade::InputArchiveType ArchiveType>
bool Serialize(ArchiveType& in_archive, ProjectConfiguration& out_project_configuration)
{
    //ARCHIVE_PARAM(in_archive, out_project_configuration, name)
    //ARCHIVE_PARAM(in_archive, out_project_configuration, description)
    //ARCHIVE_PARAM(in_archive, out_project_configuration, entry_module)
    //ARCHIVE_PARAM(in_archive, out_project_configuration, version)
    //ARCHIVE_PARAM(in_archive, out_project_configuration, chosen_implementations)
    //ARCHIVE_PARAM(in_archive, out_project_configuration, implementation_overrides)

    return true;
}

struct Project 
{
    ProjectConfiguration configuration;

    std::filesystem::path path;
};

#endif // FADE_BUILD_CONFIGURATION_PROJECT_CONFIGURATION_HPP_