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
#include "configuration/module/module_implementation.hpp"

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
#include <ranges>

static fade::application::CommandLineArgumentDescription ProjectDescription = fade::application::CommandLineArgumentDescription("p", "project", "Path to the fade project file.");
static fade::application::CommandLineArgumentDescription ModuleDirectoryDescription = fade::application::CommandLineArgumentDescription("fmd", "fade-module-directory", "One or more paths to directories containing fade modules.");
static fade::application::CommandLineArgumentDescription GenerateModuleConfigDescription = fade::application::CommandLineArgumentDescription("gmc", "generate-module-config", "Whether this application should generate the module config file. Uses user input");
static fade::application::CommandLineArgumentDescription ModuleConfigFileDescription = fade::application::CommandLineArgumentDescription("mcf", "module-config-file", "The name of the module config file.");
static fade::application::CommandLineArgumentDescription GenerateBuildFileDescription = fade::application::CommandLineArgumentDescription("gbf", "generate-build-file", "Whether this application sohuld generate a build file");
static fade::application::CommandLineArgumentDescription BuildConfigurationDescription = fade::application::CommandLineArgumentDescription("bc", "build-configuration", "Which configuration should be used.");

FadeBuild::FadeBuild() :
#ifdef FADE_PLATFORM_LINUX
    platform_(Platform::kLinux)
#elifdef FADE_PLATFORM_WINDOWS
    platform_(Platform::kWindows)
#elifdef FADE_PLATFORM_MAC
    platform_(Platform::kMac)
#elifdef FADE_PLATFORM_ANDROID
    platform_(Platform::kAndroid)
#else
    platform_(Platform::kUnknown)
#endif
{
    
}

struct Edge
{
    const struct Node* to;

    Version version;
};

struct Node
{
    const Module* module_ptr;

    const ModuleImplementation* implementation_ptr;

    fade::DynamicArray<Edge> edges;
};

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
 *      
 * Chosen Implementation Configuration
 * 
 * This should be the first step when working with the fade framework. This step will generate a build configuration file based on the user's input.
 * This step will only be executed when the --generate-module-config (or -gmc) flag is present. It will either use the file specified in --module-config-file (or -mcf).
 * First it will ask the user what entry module and implementation they would like to use, from there on it will follow the dependencies of each module and implementation until all dependencies have been fulfilled.
 *
 * 
 * Build file generation
 * 
 * DISCLAIMER: As of now, this application only generates build files for the ninja build system.
 * This step uses the loaded modules and the selected implementation based on the configuration in the project file, this step will gather all the source file locations and generate a build/project file. 
 * This final step is dependent on the --build-file-type (or -bft) flag. However, as is stated above, the system currently only supports ninja build files.
 */
void FadeBuild::Entry(const fade::application::CommandLineArguments& in_args)
{
    // Get project configuration    
    if (!GetProjectConfiguration(in_args))
    {
        fade::Log<fade::LogLevel::kError>("Error trying to get project configuration.");
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

    // Gather all the modules and their implementations
    if (!GatherFadeModules(fade_module_dirs, project_.path, fade_modules_))
    {
        fade::Log<fade::LogLevel::kError>("Failed to gather fade modules from specified directories.");
        return;
    }

    // If our project has no implementation descriptions, we'll prompt the user to configure it
    if (!project_.configuration.HasImplementationDescriptions())
    {
        // Not implemented yet
    }

    if (const fade::application::CommandLineArgument* generate_build_file_arg = in_args.Get(GenerateBuildFileDescription); generate_build_file_arg != nullptr)
    {
        // Now we create the dependency graph
        fade::DynamicArray<Node> modules_to_build;
        if (!GenerateDependencyGraph(modules_to_build, in_args))
        {
            fade::Log<fade::LogLevel::kError>("Generation of dependency graph failed.");
            return;
        }
    }
}

const std::string_view FadeBuild::GetApplicationName() const
{
    static std::string application_string = std::string("Fade Build");
    return application_string;
}

bool FadeBuild::GetProjectConfiguration(const fade::application::CommandLineArguments& in_args)
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

        if (!Serialize(json_input_archive, project_.configuration))
        {
            fade::Log<fade::LogLevel::kError>("Failed to load project configuration from file at path '{}'.", project_file_path.string());
            return false;
        }

        project_.path = project_file_path.remove_filename();

        return true;
        
    }

    return false;
}

bool TryFindModuleImplementation(const std::filesystem::path& in_directory, fade::DynamicArray<ModuleImplementation>& out_project_implementations)
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
        
            ModuleImplementation implementation { .filename = dir_entry.path().stem().string(), .path = in_directory };

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

bool FadeBuild::GetProjectImplementations(std::filesystem::path in_project_path, fade::DynamicArray<ModuleImplementation>& out_project_implementations)
{
    in_project_path.append("implementations");
    if (!std::filesystem::exists(in_project_path))
    {
        return false;
    }

    for (const std::filesystem::directory_entry& dir_entry : std::filesystem::directory_iterator(in_project_path))
    {
        if (std::filesystem::is_directory(dir_entry))
        {
            TryFindModuleImplementation(dir_entry.path(), out_project_implementations);
        }
    }

    return out_project_implementations.size() > 0;
}

bool FadeBuild::GetFadeModuleDirectories(const fade::application::CommandLineArguments& in_args, fade::DynamicArray<std::filesystem::path>& out_fade_module_dirs)
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

bool FadeBuild::FindModuleInDirectory(const std::filesystem::path& in_module_dir, std::filesystem::path& out_module_config_path)
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

void FadeBuild::GatherModuleImplementations(Module& in_out_module)
{
    const std::filesystem::path& module_implementations_path = in_out_module.module_path.string() + "/implementations";
    if (std::filesystem::exists(module_implementations_path))
    {
        for (const std::filesystem::directory_entry& dir_entry : std::filesystem::directory_iterator(module_implementations_path))
        {
            if (std::filesystem::is_directory(dir_entry))
            {
                TryFindModuleImplementation(dir_entry.path(), in_out_module.implementations);
            }
        }
    }
}

/**
 * 
 */
bool FadeBuild::GatherFadeModules(const fade::DynamicArray<std::filesystem::path>& in_fade_module_dirs, const std::filesystem::path& in_project_path, fade::DynamicArray<Module>& out_found_modules)
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
                    module.filename = module_config_path.stem().string();
                    module.module_path = dir_entry.path();
                    if (!Serialize(json_input_archive, module.configuration))
                    {
                        fade::Log<fade::LogLevel::kWarning>("Failed to load module configuration from file at path '{}'. Skipping module.", module_config_path.string());
                        continue;
                    }

                    GatherModuleImplementations(module);

                    out_found_modules.push_back(std::move(module));
                }
            }
        }
    }

    // Get the implementations of this project
    fade::DynamicArray<ModuleImplementation> project_implementations;
    if (!GetProjectImplementations(in_project_path, project_implementations))
    {
        fade::Log<fade::LogLevel::kError>("Project does not have any implementations, please implement at least one module to get started!");
    }

    // Match the project's implementations to the modules we've found
    for (fade::int32 i = static_cast<fade::int32>(project_implementations.size()) - 1; i >= 0; --i)
    {
        ModuleImplementation& implementation = project_implementations[i];
        for (Module& module : out_found_modules)
        {
            if (module.filename == implementation.configuration.implements_interface)
            {
                module.implementations.push_back(std::move(implementation));
                project_implementations.RemoveAtSwap(i);
                break;
            }
        }
    }

    // If we still have implementations, this means we're not including their module
    if (project_implementations.size() > 0)
    {
        fade::Log<fade::LogLevel::kWarning>("Unable to find modules for the following implementations provided by the project:");
        for (const ModuleImplementation& implementation : project_implementations)
        {
            fade::Log<fade::LogLevel::kWarning>("{}", implementation.configuration.name);
        }
    }

    return out_found_modules.size() > 0;
}

void AddDependenciesToStack(const fade::DynamicArray<ModuleDependency>& in_dependencies, std::vector<std::string>& out_module_stack, const fade::DynamicArray<Node>& in_nodes)
{
    for (const ModuleDependency& dependency: in_dependencies)
    {
        const std::string module_name = dependency.name;
        const auto node_find_pred = [module_name](const Node& in_node)
        {
            return in_node.module_ptr != nullptr ? module_name == in_node.module_ptr->filename : false;
        };

        // If we already have a module with this name in our list of nodes. Then we'll skip it
        if (std::find_if(std::begin(in_nodes), std::end(in_nodes), node_find_pred) != std::end(in_nodes))
        {
            continue;
        }

        const auto stack_find_pred = [module_name](const std::string& in_string)
        {
            return in_string == module_name;
        };

        // We should also check our stack to see if it contains the dependency already
        if (std::find_if(std::begin(out_module_stack), std::end(out_module_stack), stack_find_pred) != std::end(out_module_stack))
        {
            continue;
        }

        // now since we couldn't find it, we can add it to the stack.
        out_module_stack.push_back(dependency.name);
    }
}

bool CheckVersionCompatibility(const Version* in_source_version, const Module* in_source_module, const Version* in_target_version,  const Module* in_target_module)
{
    if (in_source_version != nullptr)
    {
        // If the found module doesn't have an include config, we log an error and get out
        if (in_target_version == nullptr)
        {
            fade::Log<fade::LogLevel::kError>("Module {} doesn't have an include version, which module {} is expecting", in_source_module->filename, in_target_module->filename);
            return false;
        }

        // Now we check if our dependency version is compatible.
        fade::Log<fade::LogLevel::kInfo>("Checking dependency version compatibility.\n\t-source: {}\n\t-target: {}", in_source_module->filename, in_target_module->filename);
        if (!Version::IsCompatible(*in_source_version, *in_target_version))
        {
            return false;
        }
    }

    return true;
}

bool GenerateNodeEdges(Node& in_node, const fade::DynamicArray<Node>& in_nodes)
{
    const Module* node_module = in_node.module_ptr;
    if (node_module == nullptr)
    {
        fade::Log<fade::LogLevel::kError>("trying to generate node edges for an invalid module.");
        return false;
    }

    const ModuleConfiguration& node_module_config = node_module->configuration;

    // First iterate through the module's dependencies
    for (const ModuleDependency& dependency: node_module_config.dependencies)
    {
        const auto node_search_pred = [&](const Node& in_node)
        {
            return in_node.module_ptr->filename == dependency.name;
        };

        if (const auto it = std::find_if(std::begin(in_nodes), std::end(in_nodes), node_search_pred); it != std::end(in_nodes))
        {
            const Node& found_node = *it;
            const Module* found_node_module = found_node.module_ptr;
            if (found_node_module == nullptr)
            {
                fade::Log<fade::LogLevel::kError>("Unable to confirm dependency validity for module {}, module pointer is invalid", dependency.name);
                return false;
            }

            const ModuleConfiguration& found_node_module_config = found_node_module->configuration;
            if (!CheckVersionCompatibility(dependency.include_version.get(), node_module, found_node_module_config.include_version.get(), found_node_module) 
             || !CheckVersionCompatibility(dependency.interface_version.get(), node_module, found_node_module_config.interface_version.get(), found_node_module))
            {
                return false;
            }

            Edge& new_edge = in_node.edges.emplace_back();
            new_edge.to = &found_node;
        }
    }

    const ModuleImplementation* node_implementation = in_node.implementation_ptr;
    if (node_implementation != nullptr)
    {
        const ModuleImplementationConfiguration& node_implementation_config = node_implementation->configuration;
        for (const ModuleDependency& dependency: node_implementation_config.dependencies)
        {
            const auto node_search_pred = [&](const Node& in_node)
            {
                return in_node.module_ptr->filename == dependency.name;
            };

            if (const auto it = std::find_if(std::begin(in_nodes), std::end(in_nodes), node_search_pred); it != std::end(in_nodes))
            {
                const Node& found_node = *it;
                const Module* found_node_module = found_node.module_ptr;
                if (found_node_module == nullptr)
                {
                    fade::Log<fade::LogLevel::kError>("Unable to confirm dependency validity for module {}, module pointer is invalid", dependency.name);
                    return false;
                }

                const ModuleConfiguration& found_node_module_config = found_node_module->configuration;
                if (!CheckVersionCompatibility(dependency.include_version.get(), node_module, found_node_module_config.include_version.get(), found_node_module) 
                 || !CheckVersionCompatibility(dependency.interface_version.get(), node_module, found_node_module_config.interface_version.get(), found_node_module))
                {
                    return false;
                }

                Edge& new_edge = in_node.edges.emplace_back();
                new_edge.to = &found_node;
            }
        }
    }    

    return true;
}

bool FadeBuild::GenerateDependencyGraph(fade::DynamicArray<Node>& out_nodes, const fade::application::CommandLineArguments& in_args)
{
    // Check if we even have an entry module configured
    if (project_.configuration.entry_module.empty())
    {
        fade::Log<fade::LogLevel::kError>("Unable to generate dependency graph, no entry module has been configured in the project");
        return false;
    }

    // Create the stack of modules we want to look for and add the entry module as the first to look for
    std::vector<std::string> module_stack;
    module_stack.push_back(project_.configuration.entry_module);

    std::string build_config = "";
    if (const fade::application::CommandLineArgument* build_config_arg = in_args.Get(BuildConfigurationDescription); build_config_arg != nullptr)
    {
        if (build_config_arg->values.size() > 0)
        {
            build_config = build_config_arg->values[0];
        }
    }

    fade::DynamicArray<ModuleImplementationPair> implementations = GetImplementations(build_config);

    Node* previous_node = nullptr;
    bool generation_failed = false;

    while (!module_stack.empty())
    {
        // Get the module on top of the stack
        const std::string& module_to_find = module_stack.back();
        if (module_to_find.empty())
        {
            // This code should technically never be called, but just in case we check whether the string is empty.
            fade::Log<fade::LogLevel::kWarning>("Empty module was pushed onto the search stack");
            continue;
        }

        const auto pred = [module_to_find](const Module& in_module) 
        {
            return in_module.filename == module_to_find;            
        };

        const auto it = std::find_if(std::begin(fade_modules_), std::end(fade_modules_), pred);
        if (it == std::end(fade_modules_))
        {
            if (previous_node != nullptr)
            {
                fade::Log<fade::LogLevel::kError>("Unable to find module {}, which is a dependency of {}{}"
                    , previous_node->module_ptr != nullptr ? previous_node->module_ptr->filename : "unknown"
                    , previous_node->implementation_ptr != nullptr ? ":" + previous_node->module_ptr->filename : "");
            }
            else
            {
                fade::Log<fade::LogLevel::kError>("Unable to find module {}");
            }
            
            generation_failed |= true;
            continue;
        }

        // Now that we have a module, let's create the node
        Node& new_node = out_nodes.emplace_back();
        new_node.module_ptr = &(*it);

        // If we have only a single implementation, we just use that
        if (new_node.module_ptr->implementations.size() == 1)
        {
            new_node.implementation_ptr = &new_node.module_ptr->implementations[0];
        }
        // If we have multiple, we will try and find the right one based on either the build configuration or platform
        else if (new_node.module_ptr->implementations.size() > 1)
        {
            const auto pair_search_pred = [module_to_find](const ModuleImplementationPair& in_pair)
            {
                return in_pair.module == module_to_find;
            };

            // First we try to see if our build configuration specifies an implementation that should be used
            if (auto pair_it = std::find_if(std::begin(implementations), std::end(implementations), pair_search_pred); pair_it != std::end(implementations))
            {
                const ModuleImplementationPair& found_pair = *pair_it;
                const auto implementation_search_pred = [found_pair](const ModuleImplementation& in_implementation)
                {
                    return in_implementation.filename == found_pair.implementation;
                };

                if (const auto impl_it = std::find_if(std::begin(new_node.module_ptr->implementations), std::end(new_node.module_ptr->implementations), implementation_search_pred); impl_it != std::end(new_node.module_ptr->implementations))
                {
                    new_node.implementation_ptr = &(*impl_it);
                }
            }
            // If we couldn't find one specified by the build config, we will check if it should be based on
            else if (new_node.module_ptr->configuration.has_platform_implementations)
            {
                const std::string platform_string = fade::EnumToString(platform_);
                const auto implementation_search_pred = [platform_string](const ModuleImplementation& in_implementation)
                {
                    return in_implementation.filename == platform_string;
                };

                if (const auto impl_it = std::find_if(std::begin(new_node.module_ptr->implementations), std::end(new_node.module_ptr->implementations), implementation_search_pred); impl_it != std::end(new_node.module_ptr->implementations))
                {
                    new_node.implementation_ptr = &(*impl_it);
                }
            }
            // Finally, we log an error if there is no way for us to figure out which implementation we should use
            else 
            {
                fade::Log<fade::LogLevel::kError>("Unable to find which implementation to use for module {}", new_node.module_ptr->filename);
                generation_failed |= true;
            }
        }

        module_stack.pop_back();

        // Now let's find out which dependencies we don't have a node for yet, and add them to the search stack
        if (new_node.implementation_ptr != nullptr)
        {
            AddDependenciesToStack(new_node.implementation_ptr->configuration.dependencies, module_stack, out_nodes);
        }
        
        AddDependenciesToStack(new_node.module_ptr->configuration.dependencies, module_stack, out_nodes);

        previous_node = &new_node;
    }

    // Early out, for when we've already had an issue with generating the dependency graph.
    if (generation_failed)
    {
        return false;
    }

    // Now that we've gathered all the module nodes, let's configure the edges.
    for (Node& node: out_nodes)
    {
        generation_failed |= !GenerateNodeEdges(node, out_nodes);
    }

    return !generation_failed;
}

bool FadeBuild::FindPlatformImplementation(const Module* in_module, const ModuleImplementation*& out_module_implementation) const
{
    assert(in_module != nullptr);
    assert(platform_ != Platform::kUnknown);

    for (const ModuleImplementation& implementation: in_module->implementations)
    {
        if (implementation.configuration.platform == platform_)
        {
            out_module_implementation = &implementation;
            break;
        }
    }

    if (out_module_implementation == nullptr)
    {
        fade::Log<fade::LogLevel::kError>("Unable to find platform implementation {} for module {}", in_module->filename, fade::EnumToString(platform_));
        return false;
    }

    return true;
}

fade::DynamicArray<ModuleImplementationPair> FadeBuild::GetImplementations(const std::string& in_build_config) const
{
    const ProjectConfiguration& configuration = project_.configuration;
    fade::DynamicArray<ModuleImplementationPair> implementations = configuration.chosen_implementations;
    if (!in_build_config.empty())
    {
        const auto description_search_pred = [in_build_config](const ImplementationDescription& in_description) 
        {
            return in_description.name == in_build_config;
        };

        if (auto descr_it = std::find_if(std::begin(configuration.implementation_overrides), std::end(configuration.implementation_overrides), description_search_pred); descr_it != std::end(configuration.implementation_overrides))
        {
            const ImplementationDescription& implementation_description = *descr_it;
            for (const ModuleImplementationPair& pair : implementation_description.module_implementation_pairs)
            {            
                const auto pair_search_pred = [pair](const ModuleImplementationPair& in_pair)
                {
                    return pair.module == in_pair.module;
                };
                
                if (auto pair_it = std::find_if(std::begin(implementations), std::end(implementations), pair_search_pred); pair_it != std::end(implementations))
                {
                    *pair_it = pair;
                }
            }
        }
    }
    return implementations;
}

namespace fade::application {

std::unique_ptr<ApplicationBase> Create()
{
    return std::make_unique<FadeBuild>();
}

}