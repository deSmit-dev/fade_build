#include "fade_build.hpp"

// Fade includes
#include "core/include/containers/dynamic_array.hpp"
#include "core/include/serialization/input_archive.hpp"
#include "core/include/serialization/json_input_archive.hpp"
#include "core/include/logging.hpp"
#include "core/include/type_definitions.hpp"

// Fade Build includes
#include "configuration/version.hpp"
#include "configuration/module/module.hpp"
#include "configuration/module/module_configuration.hpp"
#include "configuration/module/module_implementation.hpp"
#include "configuration/project/project.hpp"

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

bool GetProjectConfiguration(const fade::application::CommandLineArguments& in_args, ProjectConfiguration& out_project_configuration, std::filesystem::path& out_project_path)
{
    const auto GetProjectFilePath = [](const fade::application::CommandLineArguments& in_args, std::filesystem::path& out_path) -> bool
    {
        if (const fade::application::CommandLineArgument* project_arg = in_args.Get("p", "project"); project_arg != nullptr)
        {
            if (project_arg->values.size() < 1)
            {
                fade::Log<fade::LogLevel::kError>("No project file specified.");
                return false;
            }

            if (project_arg->values.size() > 1)
            {
                fade::Log<fade::LogLevel::kWarning>("Multiple project files specified. Only the first one will be used.");
            }

            out_path = project_arg->values[0];
            // check for project path validity
            if (out_path.string().contains(".fproject") == false)
            {
                fade::Log<fade::LogLevel::kError>("{}. Must have .fproject extension.", out_path.string());
                return false;
            }
        }

        return true;
    };

    if (std::filesystem::path project_file_path; GetProjectFilePath(in_args, project_file_path))
    {
        if (project_file_path.empty())
        {
            fade::Log<fade::LogLevel::kError>("Specified project file path is empty.");
            return false;
        }

        if (project_file_path.is_relative())
        {
            project_file_path = std::filesystem::absolute(project_file_path).lexically_normal();
        }

        if (!std::filesystem::exists(project_file_path))
        {
            fade::Log<fade::LogLevel::kError>("Project file path does not exist {}.", project_file_path.string());
            return false;
        }
        
        fade::Log<fade::LogLevel::kInfo>("Using project file: {}", project_file_path.string());

        // Load project configuration
        std::ifstream project_file_stream(project_file_path, std::ios::in);
        fade::JsonInputArchive json_input_archive;
        if (!json_input_archive.Parse(project_file_stream))
        {
            fade::Log<fade::LogLevel::kError>("Failed to parse project file at path '{}'.", project_file_path.string());
            return false;
        }

        if (!Serialize(json_input_archive, out_project_configuration))
        {
            fade::Log<fade::LogLevel::kError>("Failed to load project configuration from file at path '{}'.", project_file_path.string());
            return false;
        }

        out_project_path = project_file_path.remove_filename();

        return true;
        
    }

    return false;
}

bool GetProjectImplementations(std::filesystem::path in_project_path, fade::DynamicArray<ModuleImplementation>& out_project_implementations)
{
    in_project_path.append("implementations");
    if (!std::filesystem::exists(in_project_path))
    {
        return false;
    }

    auto TryFindModuleImplementation = [](const std::filesystem::path& in_directory, fade::DynamicArray<ModuleImplementation>& out_project_implementations)
    {
        for (const std::filesystem::directory_entry& dir_entry : std::filesystem::directory_iterator(in_directory))
        {
            const std::string path_string = dir_entry.path().string();
            if (dir_entry.is_regular_file() && std::regex_match(path_string, std::regex(".*.fimpl")))
            {
                std::ifstream project_file_stream(dir_entry.path(), std::ios::in);
                fade::JsonInputArchive json_input_archive;
                if (!json_input_archive.Parse(project_file_stream))
                {
                    fade::Log<fade::LogLevel::kError>("Failed to parse implementation file at path '{}'.", path_string);
                    continue;
                }
            
                ModuleImplementation implementation { .path = in_directory };

                if (!Serialize(json_input_archive, implementation.configuration))
                {
                    fade::Log<fade::LogLevel::kError>("Failed to load implementation from file at path '{}'.", path_string);
                    continue;
                }

                out_project_implementations.push_back(std::move(implementation));
            }
        }

        return out_project_implementations.size() > 0;
    };

    for (const std::filesystem::directory_entry& dir_entry : std::filesystem::directory_iterator(in_project_path))
    {
        if (std::filesystem::is_directory(dir_entry))
        {
            TryFindModuleImplementation(dir_entry.path(), out_project_implementations);
        }
    }

    return out_project_implementations.size() > 0;
}

bool GetFadeModuleDirectories(const fade::application::CommandLineArguments& in_args, fade::DynamicArray<std::filesystem::path>& out_fade_module_dirs)
{
    if (char* fade_module_dir_env = std::getenv("FADE_MODULE_DIR"); fade_module_dir_env != nullptr)
    {
        if (std::filesystem::path fade_module_dir = std::filesystem::path(fade_module_dir_env); !fade_module_dir.empty() && std::filesystem::exists(fade_module_dir))
        {
            out_fade_module_dirs.push_back(fade_module_dir);
        }
        else
        {
            fade::Log<fade::LogLevel::kWarning>("Environment variable FADE_MODULE_DIR is set to '{}', but the directory does not exist.", fade_module_dir_env);
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
                fade::Log<fade::LogLevel::kWarning>("Fade module directory '{}' does not exist or has already been added.", dir_str);
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

bool GatherFadeModules(const fade::DynamicArray<std::filesystem::path>& in_fade_module_dirs, fade::DynamicArray<Module>& out_found_modules)
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
                    fade::JsonInputArchive json_input_archive;
                    if (!json_input_archive.Parse(module_config_stream))
                    {
                        fade::Log<fade::LogLevel::kWarning>("Failed to parse module configuration file at path '{}'. Skipping module.", module_config_path.string());
                        continue;
                    }

                    Module module;
                    module.module_file_name = dir_entry.path().filename().string();
                    module.module_path = dir_entry.path();
                    if (!Serialize(json_input_archive, module.configuration))
                    {
                        fade::Log<fade::LogLevel::kWarning>("Failed to load module configuration from file at path '{}'. Skipping module.", module_config_path.string());
                        continue;
                    }

                    out_found_modules.push_back(std::move(module));
                }
            }
        }
    }

    return out_found_modules.size() > 0;
}

bool GenerateModuleConfigFile(const ProjectConfiguration& in_project_configuration, const fade::DynamicArray<ModuleImplementation>& in_project_implementations, const fade::DynamicArray<Module>& in_found_modules, ModuleConfiguration& out_module_configuration)
{


    return true;
}

static fade::application::CommandLineArgumentDescription ProjectDescription = fade::application::CommandLineArgumentDescription("p", "project", "Path to the fade project file.");
static fade::application::CommandLineArgumentDescription ModuleDirectoryDescription = fade::application::CommandLineArgumentDescription("fmd", "fade-module-directory", "One or more paths to directories containing fade modules.");
static fade::application::CommandLineArgumentDescription GenerateModuleConfigDescription = fade::application::CommandLineArgumentDescription("gmc", "generate-module-config", "Whether this application should generate the module config file. Uses user input");
static fade::application::CommandLineArgumentDescription ModuleConfigFileDescription = fade::application::CommandLineArgumentDescription("mcf", "module-config-file", "The name of the module config file.");
static fade::application::CommandLineArgumentDescription GenerateBuildFileDescription = fade::application::CommandLineArgumentDescription("gbf", "generate-build-file", "Whether this application sohuld generate a build file");

/**
 * Project file explanation
 * 
 * The fade framework uses a few different kinds of json formatted files to describe projects, modules and their implementations.
 * 
 * The project file (.fproject) describes the overall project including the name
 * A project itself should have an implementation for one or more modules. 
 * The implementation may also be for the user's own modules. Meaning the user doesn't have to use any of the modules provided by the fade framework to make their application.
 * In which case, this is just another overly simple build file generation system, at which point the user should probably just use something like CMake.
 * 
 * The module files (.fmodule) describe the module itself, including its name, version and interface and/or source configurations. 
 * A module must have either an interface or source configuration or both.
 * When a module is supposed to implement a main function, this should be reflected in the configuration file.
 * 
 * Implementation configuration files (.fimpl) describe the implementation, in this configuration the user should specifiy which module it implements. 
 * However, if the implementation is defined within the folder hierarchy of the module, this isn't necessary.
 * 
 * Finally we have the module configuration files (.fmconfig) which describe what platform and build system should be used, and are used to determine which implementations should be used for all the modules used by the project.
 * This application should assist the user in creating their first module configuration file. 
 * 
 *      
 * Module config file generation
 * 
 * This should be the first step when working with the fade framework. This step will generate a build configuration file based on the user's input.
 * This step will only be executed when the --generate-module-config (or -gmc) flag is present. It will either use the file specified in --module-config-file (or -mcf).
 * First it will ask the user what entry module and implementation they would like to use, from there on it will follow the dependencies of each module and implementation until all dependencies have been fulfilled.
 *
 * 
 * Build file generation
 * 
 * DISCLAIMER: As of now, this application only generates build files for the ninja build system.
 * This step will only be executed when the --generate-build-file (or -gbf) flag is present. 
 * It will either use the module config file specified in --module-config-file (or -mcf) or, the file name that was selected by the module config file generation.
 * Once the configuration has been loaded, it will either load the necessary modules from disk or use the result of the module config file generation step.
 * Finally, using the loaded modules and their configuration, this step will gather all the source file locations and generate a build/project file. 
 * This final step is dependent on the --build-file-type (or -bft) flag. However, as is stated above, the system currently only supports ninja build files.
 */
void FadeBuild::Entry(const fade::application::CommandLineArguments& in_args)
{
    // Get project configuration
    ProjectConfiguration project_config;
    std::filesystem::path project_path;
    if (!GetProjectConfiguration(in_args, project_config, project_path))
    {
        fade::Log<fade::LogLevel::kError>("Error trying to get project configuration.");
        return;
    }
    
    // Find implementations for this project
    fade::DynamicArray<ModuleImplementation> project_implementations;
    if (!GetProjectImplementations(project_path, project_implementations))
    {
        fade::Log<fade::LogLevel::kError>("No implementations found in this project.");
        return;
    }

    // Get the directories where we can find the modules
    fade::DynamicArray<std::filesystem::path> fade_module_dirs;
    if (!GetFadeModuleDirectories(in_args, fade_module_dirs))
    {
        fade::Log<fade::LogLevel::kError>("No fade module directories found.");
        return;
    }

    fade::Log<fade::LogLevel::kInfo>("Using {} fade module {}:", fade_module_dirs.size(), fade_module_dirs.size() == 1 ? "directory" : "directories");
    for (const std::filesystem::path& module_dir : fade_module_dirs)
    {
        fade::Log<fade::LogLevel::kVerbose>("- {}", module_dir.string());
    }

    // Gather all the modules
    fade::DynamicArray<Module> found_modules;
    if (!GatherFadeModules(fade_module_dirs, found_modules))
    {
        fade::Log<fade::LogLevel::kError>("Failed to gather fade modules from specified directories.");
        return;
    }

    // If we need to generate the module configuration for this project, we do so now
    ModuleConfiguration module_configuration;
    if (const fade::application::CommandLineArgument* generate_module_config = in_args.Get(GenerateModuleConfigDescription); generate_module_config != nullptr)
    {
        if (!GenerateModuleConfigFile(project_config, project_implementations, found_modules, module_configuration))
        {
            fade::Log<fade::LogLevel::kError>("Error trying to generate module config file");
            return;
        }
    }
    
    if (const fade::application::CommandLineArgument* generate_build_file = in_args.Get(GenerateBuildFileDescription); generate_build_file != nullptr)
    {
        if (module_configuration.IsValid())
        {
            
        }
    }

    fade::Log<fade::LogLevel::kInfo>("Generating project files for project '{}', version '{}'.", project_config.name, project_config.version.ToString());

    fade::Log<fade::LogLevel::kInfo>("Found {} implementations.", project_implementations.size());
    for (const ModuleImplementation& impl : project_implementations)
    {
        const std::size_t num_dependencies = impl.configuration.dependencies.size();
        fade::Log<fade::LogLevel::kVerbose>("{}\n\tDescription: {}\n\tImplements interface: {}\n\tPlatform: {}\n\tDependencies: {}"
            , impl.configuration.name
            , impl.configuration.description
            , impl.configuration.implements_interface
            , EnumToString(impl.configuration.platform)
            , num_dependencies);
    }

    fade::Log<fade::LogLevel::kInfo>("Found {} {}.", found_modules.size(), found_modules.size() == 1 ? "module" : "modules");
    for (const Module& module : found_modules)
    {
        fade::Log<fade::LogLevel::kVerbose>("{}\n\tinterface: {}\n\tinclude: {}\n\tImplements entry: {}"
            , module.configuration.name
            , module.configuration.interface_config != nullptr ? module.configuration.interface_config->version.ToString() : "null"
            , module.configuration.include_config != nullptr ? module.configuration.include_config->version.ToString() : "null"
            , module.configuration.implements_main
        );
    }
}

const std::string_view FadeBuild::GetApplicationName() const
{
    static std::string application_string = std::string("Fade Build");
    return application_string;
}

namespace fade::application {

std::unique_ptr<ApplicationBase> Create()
{
    return std::make_unique<FadeBuild>();
}

}