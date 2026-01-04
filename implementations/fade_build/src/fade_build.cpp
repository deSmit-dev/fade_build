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
#include <regex>
#include <cassert>

namespace fade::core {
class OutputArchive
{

};
}

// Used, for the time being, to prepare for reflection. 
// Since we would no longer require to manually write the name of the field with reflection.
#define ARCHIVE_PARAM(archive, obj, field) \
if constexpr (std::derived_from<std::remove_reference_t<decltype(archive)>, fade::core::OutputArchive>) {\
    in_archive << fade::core::const_name_value_pair<std::remove_reference_t<decltype(obj.field)>>(#field, &obj.field); \
} else if constexpr (std::derived_from<std::remove_reference_t<decltype(archive)>, fade::core::InputArchive>) { \
    in_archive << fade::core::name_value_pair<std::remove_reference_t<decltype(obj.field)>>(#field, &obj.field); \
}

struct Version
{
    int major = 0;
    int minor = 0;
    int revision = 0;

    std::string ToString() const 
    {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(revision);
    }

    bool operator==(const Version& other) const
    {
        return major == other.major && minor == other.minor && revision == other.revision;
    }

    bool operator<(const Version& in_rhs) const
    {
        assert(false);
        return true;    
    }

    bool operator>(const Version& in_rhs) const
    {
        assert(false);
        return true;
    }
};

template <typename ArchiveType>
bool Serialize(ArchiveType& in_archive, Version& in_version)
    requires(fade::core::IsInputArchiveClass<ArchiveType>)
{
    ARCHIVE_PARAM(in_archive, in_version, major)
    ARCHIVE_PARAM(in_archive, in_version, minor)
    ARCHIVE_PARAM(in_archive, in_version, revision)
    return true;
}

struct ProjectApplicationModule
{
    std::string implementation;
    std::string parent_implementation;
};


namespace fade::core {

template <typename ArchiveType>
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
    Version version;

    std::string entry_module_implementation;

    ProjectApplicationModule application_module;

    bool operator==(const ProjectConfiguration& other) const
    {
        return name == other.name && version == other.version;
    }
};

struct ModuleInterfaceConfiguration
{
    Version version;
};

template <typename ArchiveType>
bool Serialize(ArchiveType& in_archive, ModuleInterfaceConfiguration& in_out_module_interface_config)
    requires(fade::core::IsInputArchiveClass<ArchiveType>)
{
    return true;
}

struct ModuleIncludeConfiguration
{
    Version version;
};

template <typename ArchiveType>
bool Serialize(ArchiveType& in_archive, ModuleIncludeConfiguration& in_out_module_include_config)
    requires(fade::core::IsInputArchiveClass<ArchiveType>)
{    
    ARCHIVE_PARAM(in_archive, in_out_module_include_config, version);
    return true;
}

/**
 * Module Configuration
 * 
 * Some basic metadata to describe the module.
 */
struct ModuleConfiguration
{
    // The name of the module
    std::string name;
    // The description of the module
    std::string description;
    // The optional interface metadata of this module
    std::shared_ptr<ModuleInterfaceConfiguration> interface_config;
    // The optional source metadata of this module
    std::shared_ptr<ModuleIncludeConfiguration> include_config;
};

template <typename ArchiveType>
bool Serialize(ArchiveType& in_archive, ModuleConfiguration& in_out_module_configuration)
    requires(fade::core::IsInputArchiveClass<ArchiveType>)
{
    ARCHIVE_PARAM(in_archive, in_out_module_configuration, name)
    ARCHIVE_PARAM(in_archive, in_out_module_configuration, description)
    ARCHIVE_PARAM(in_archive, in_out_module_configuration, interface_config)
    ARCHIVE_PARAM(in_archive, in_out_module_configuration, include_config)
    return true;
}

struct ModuleImplementationConfiguration
{

};

template <typename ArchiveType>
bool Serialize(ArchiveType& in_archive, ModuleImplementationConfiguration& in_module_implementation)
    requires(fade::core::IsInputArchiveClass<ArchiveType>)
{
    return true;
}

struct ModuleImplementation
{
    ModuleImplementationConfiguration configuration;
    std::filesystem::path path;

};

struct ModuleInterface
{
    std::vector<ModuleImplementation> implementations;
};

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
    ModuleConfiguration configuration;
    struct ModuleInterface interface;
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
    ARCHIVE_PARAM(in_archive, in_project_configuration, application_module);

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

bool GetProjectConfiguration(const fade::application::CommandLineArguments& in_args, ProjectConfiguration& out_project_configuration)
{
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
            return false;
        }

        if (project_file_path.is_relative())
        {
            project_file_path = std::filesystem::absolute(project_file_path).lexically_normal();
        }

        if (!std::filesystem::exists(project_file_path))
        {
            fade::core::Log<fade::core::LogLevel::kError>("Project file path does not exist {}.", project_file_path.string());
            return false;
        }
        
        fade::core::Log<fade::core::LogLevel::kInfo>("Using project file: {}", project_file_path.string());

        // Load project configuration
        std::ifstream project_file_stream(project_file_path, std::ios::in);
        fade::core::JsonInputArchive json_input_archive;
        if (!json_input_archive.Parse(project_file_stream))
        {
            fade::core::Log<fade::core::LogLevel::kError>("Failed to parse project file at path '{}'.", project_file_path.string());
            return false;
        }

        if (!Serialize(json_input_archive, out_project_configuration))
        {
            fade::core::Log<fade::core::LogLevel::kError>("Failed to load project configuration from file at path '{}'.", project_file_path.string());
            return false;
        }

        return true;
        
    }

    return false;
}

bool GetFadeModuleDirectories(const fade::application::CommandLineArguments& in_args, std::vector<std::filesystem::path>& out_fade_module_dirs)
{
    if (char* fade_module_dir_env = std::getenv("FADE_MODULE_DIR"); fade_module_dir_env != nullptr)
    {
        if (std::filesystem::path fade_module_dir = std::filesystem::path(fade_module_dir_env); !fade_module_dir.empty() && std::filesystem::exists(fade_module_dir))
        {
            out_fade_module_dirs.push_back(fade_module_dir);
        }
        else
        {
            fade::core::Log<fade::core::LogLevel::kWarning>("Environment variable FADE_MODULE_DIR is set to '{}', but the directory does not exist.", fade_module_dir_env);
        }
    }

    if (const fade::application::CommandLineArgument* fade_module_dirs_arg = in_args.Get("fmd", "fade_module_dirs"); fade_module_dirs_arg != nullptr)
    {
        for (const std::string& dir_str : fade_module_dirs_arg->values)
        {
            std::filesystem::path dir_path(dir_str);
            if (std::filesystem::exists(dir_path) && std::find_if(out_fade_module_dirs.begin(), out_fade_module_dirs.end(), 
                [dir_path](const std::filesystem::path& existing_path)
                {
                    return existing_path == dir_path;
                }) == out_fade_module_dirs.end())
            {
                out_fade_module_dirs.push_back(dir_path);
            }
            else
            {
                fade::core::Log<fade::core::LogLevel::kWarning>("Fade module directory '{}' does not exist or has already been added.", dir_str);
            }
        }
    }

    return out_fade_module_dirs.size() > 0;
}

bool FindModuleInDirectory(const std::filesystem::path& in_module_dir, std::filesystem::path& out_module_config_path)
{
    for (const std::filesystem::directory_entry& dir_entry : std::filesystem::directory_iterator(in_module_dir))
    {
        if (dir_entry.is_regular_file() && std::regex_match(dir_entry.path().string(), std::regex(".*.fmodule")))
        {
            out_module_config_path = dir_entry.path();
            return true;
        }
    }

    return false;
}

bool GatherFadeModules(const std::vector<std::filesystem::path>& in_fade_module_dirs, std::vector<Module>& out_found_modules)
{
    for (const std::filesystem::path& module_dir : in_fade_module_dirs)
    {
        for (const std::filesystem::directory_entry& dir_entry : std::filesystem::directory_iterator(module_dir))
        {
            if (dir_entry.is_directory())
            {
                std::filesystem::path module_config_path;
                if (FindModuleInDirectory(dir_entry.path(), module_config_path))
                {
                    std::ifstream module_config_stream(module_config_path, std::ios::in);
                    fade::core::JsonInputArchive json_input_archive;
                    if (!json_input_archive.Parse(module_config_stream))
                    {
                        fade::core::Log<fade::core::LogLevel::kWarning>("Failed to parse module configuration file at path '{}'. Skipping module.", module_config_path.string());
                        continue;
                    }

                    Module module;
                    if (!Serialize(json_input_archive, module.configuration))
                    {
                        fade::core::Log<fade::core::LogLevel::kWarning>("Failed to load module configuration from file at path '{}'. Skipping module.", module_config_path.string());
                        continue;
                    }

                    out_found_modules.push_back(module);
                }
            }
        }
    }

    return true;
}

void FadeBuild::Entry(const fade::application::CommandLineArguments& in_args)
{
    if (in_args.Get("h", "help") != nullptr)
    {
        PrintHelpString();
        return;
    }

    ProjectConfiguration project_config;
    if (!GetProjectConfiguration(in_args, project_config))
    {

        return;
    }
   
    fade::core::Log<fade::core::LogLevel::kInfo>("Generating project files for project '{}', version '{}'.", project_config.name, project_config.version.ToString());

    std::vector<std::filesystem::path> fade_module_dirs;
    if (!GetFadeModuleDirectories(in_args, fade_module_dirs))
    {
        fade::core::Log<fade::core::LogLevel::kWarning>("No fade module directories found.");
        return;
    }

    fade::core::Log<fade::core::LogLevel::kInfo>("Using {} fade module {}:", fade_module_dirs.size(), fade_module_dirs.size() == 1 ? "directory" : "directories");
    for (const std::filesystem::path& module_dir : fade_module_dirs)
    {
        fade::core::Log<fade::core::LogLevel::kVerbose>("\t- {}", module_dir.string());
    }

    std::vector<Module> found_modules;
    if (!GatherFadeModules(fade_module_dirs, found_modules))
    {
        fade::core::Log<fade::core::LogLevel::kError>("Failed to gather fade modules from specified directories.");
        return;
    }

    fade::core::Log<fade::core::LogLevel::kInfo>("Found {} fade {}.", found_modules.size(), found_modules.size() == 1 ? "module" : "modules");
    for (const Module& module : found_modules)
    {
        fade::core::Log<fade::core::LogLevel::kVerbose>("\t- {}\n\t\tinterface: {}\n\t\tinclude: {}"
            , module.configuration.name
            , module.configuration.interface_config != nullptr ? module.configuration.interface_config->version.ToString() : "null"
            , module.configuration.include_config != nullptr ? module.configuration.include_config->version.ToString() : "null"
        );
    }
}

namespace fade::application {

std::unique_ptr<ApplicationBase> Create()
{
    return std::make_unique<FadeBuild>();
}

}