#include "fade_build.hpp"

// Fade includes
#include "core/include/serialization/input_archive.hpp"
#include "core/include/serialization/json_input_archive.hpp"
#include "core/include/logging.hpp"
#include "core/include/type_definitions.hpp"

// STL includes
#include <iostream>
#include <sstream>
#include <print>
#include <filesystem>
#include <map>
#include <any>
#include <type_traits>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <unordered_map>

struct ProjectApplicationModule
{
    std::string implementation;
    std::string parent_implementation;
};

class OutputArchive
{

};

// Used, for the time being, to prepare for reflection. 
// Since we would no longer require to manually write the name of the field with reflection.
#define ARCHIVE_PARAM(archive, obj, field) \
if constexpr (std::derived_from<std::remove_reference_t<decltype(archive)>, OutputArchive>) {\
    in_archive << fade::core::const_name_value_pair<std::remove_reference_t<decltype(obj.field)>>(#field, &obj.field); \
} else if constexpr (std::derived_from<std::remove_reference_t<decltype(archive)>, InputArchive>) { \
    in_archive << fade::core::name_value_pair<std::remove_reference_t<decltype(obj.field)>>(#field, &obj.field); \
}

namespace fade::core {

template<typename ArchiveType>
bool Serialize(ArchiveType& in_archive, ProjectApplicationModule& in_project_application_module)
    requires(fade::core::IsInputArchiveClass<ArchiveType>)
{
    ARCHIVE_PARAM(in_archive, in_project_application_module, implementation)
    ARCHIVE_PARAM(in_archive, in_project_application_module, parent_implementation)
    return true;
}

}

struct ProjectConfiguration
{
    std::string name;
    std::string description;
    std::string version;

    std::string entry_module_implementation;

    ProjectApplicationModule application_module;

    bool operator==(const ProjectConfiguration& other) const
    {
        return name == other.name && version == other.version;
    }
};

namespace fade::core {

template <typename ArchiveType>
bool Serialize(ArchiveType& in_archive, ProjectConfiguration& in_project_configuration)
    requires(fade::core::IsInputArchiveClass<ArchiveType>)
{
    ARCHIVE_PARAM(in_archive, in_project_configuration, name)
    ARCHIVE_PARAM(in_archive, in_project_configuration, description)
    ARCHIVE_PARAM(in_archive, in_project_configuration, version)
    ARCHIVE_PARAM(in_archive, in_project_configuration, entry_module_implementation)
    ARCHIVE_PARAM(in_archive, in_project_configuration, application_module)

    return true;
}

}

void PrintHelpString()
{
    std::stringstream help_stream;
    help_stream << "Test application usage:\n";
    help_stream << "\t-h\t\tPrints this help string\n"; 
    std::print("{}", help_stream.str());
}

void FadeBuild::Entry(const fade::application::CommandLineArguments& in_args)
{
    if (in_args.Get("h", "help") != nullptr)
    {
        PrintHelpString();
        return;
    }

    const auto GetProjectFilePath = [](const fade::application::CommandLineArguments& in_args, std::filesystem::path& out_path) -> bool
    {
        if (const fade::application::CommandLineArgument* project_arg = in_args.Get("p", "project"); project_arg != nullptr)
        {
            if (project_arg->values.size() < 1)
            {
                fade::core::Log<fade::core::LogLevel::kError>("No project file specified.");
                return false;
            }

            if (project_arg->values.size() > 1)
            {
                fade::core::Log<fade::core::LogLevel::kWarning>("Multiple project files specified. Only the first one will be used.");
            }

            out_path = project_arg->values[0];
            // check for project path validity
            if (out_path.string().contains(".fproject") == false)
            {
                fade::core::Log<fade::core::LogLevel::kError>("{}. Must have .fproject extension.", out_path.string());
                return false;
            }
        }

        return true;
    };

    if (std::filesystem::path project_file_path; GetProjectFilePath(in_args, project_file_path))
    {
        if (project_file_path.empty())
        {
            fade::core::Log<fade::core::LogLevel::kError>("Specified project file path is empty.");
            return;
        }

        if (project_file_path.is_relative())
        {
            project_file_path = std::filesystem::absolute(project_file_path).lexically_normal();
        }

        if (!std::filesystem::exists(project_file_path))
        {
            fade::core::Log<fade::core::LogLevel::kError>("{}.", project_file_path.string());
            return;
        }
        
        fade::core::Log<fade::core::LogLevel::kInfo>("Using project file: {}", project_file_path.string());

        // Load project configuration
        ProjectConfiguration project_config;
        std::ifstream project_file_stream(project_file_path, std::ios::in);
        fade::core::JsonInputArchive json_input_archive;
        if (!json_input_archive.Parse(project_file_stream))
        {
            fade::core::Log<fade::core::LogLevel::kError>("Failed to parse project file at path '{}'.", project_file_path.string());
            return;
        }

        if (Serialize(json_input_archive, project_config))
        {
            fade::core::Log<fade::core::LogLevel::kInfo>("Successfully loaded project configuration.");
            fade::core::Log<fade::core::LogLevel::kInfo>("Project Name: {}", project_config.name);
            fade::core::Log<fade::core::LogLevel::kInfo>("Project Description: {}", project_config.description);
            fade::core::Log<fade::core::LogLevel::kInfo>("Project Version: {}", project_config.version);
            fade::core::Log<fade::core::LogLevel::kInfo>("Entry Module Implementation: {}", project_config.entry_module_implementation);
            fade::core::Log<fade::core::LogLevel::kInfo>("Application Module Implementation: {}", project_config.application_module.implementation);
            fade::core::Log<fade::core::LogLevel::kInfo>("Application Module Parent Implementation: {}", project_config.application_module.parent_implementation);
        }
    }
}

namespace fade::application {

std::unique_ptr<ApplicationBase> Create()
{
    return std::make_unique<FadeBuild>();
}

}