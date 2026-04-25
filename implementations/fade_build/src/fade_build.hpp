#ifndef FADE_BUILD_HPP
#define FADE_BUILD_HPP

// Parent include
#include "application/interface/application.hpp"

// Fade Build includes
#include "configuration/project/project.hpp"
#include "fade_build_types.hpp"

/**
 * Fade Build Application
 */
class FadeBuild : public fade::application::ApplicationBase
{
public:
    FadeBuild();
    
    /**
     * Entry point of the application
     */
    virtual void Entry(const fade::application::CommandLineArguments& in_args) override;

    virtual const std::string_view GetApplicationName() const override;

private:
    /**
     * Get project configuration
     * 
     * Based on the arguments passed to the application, finds, deserializes and caches the project configuration.
     */
    bool GetProjectConfiguration(const fade::application::CommandLineArguments& in_args);

    bool GetProjectImplementations(std::filesystem::path in_project_path, fade::DynamicArray<class ModuleImplementation>& out_project_implementations);

    bool GetFadeModuleDirectories(const fade::application::CommandLineArguments& in_args, fade::DynamicArray<std::filesystem::path>& out_fade_module_dirs);

    bool FindModuleInDirectory(const std::filesystem::path& in_module_dir, std::filesystem::path& out_module_config_path);

    void GatherModuleImplementations(class Module& in_out_module);

    bool GatherFadeModules(const fade::DynamicArray<std::filesystem::path>& in_fade_module_dirs, const std::filesystem::path& in_project_path, fade::DynamicArray<class Module>& out_found_modules);

    //bool GatherSingleModuleToBuild(std::stack<const class Module*> in_module_stack, const ProjectConfiguration& in_project_configuration, const fade::DynamicArray<class Module>& in_found_modules, fade::DynamicArray<struct ModuleToBuild>& out_modules_to_build);

    //bool GatherModulesToBuild(const ProjectConfiguration& in_project_configuration, const fade::DynamicArray<class Module>& in_found_modules, fade::DynamicArray<struct ModuleToBuild>& out_modules_to_build);

    /**
     * Generate Module Dependency Graph
     * 
     * Based on the modules and their implementations, generate a dependency graph.
     * - Each node is a module and implementation pair.
     * - Each edge contains version information of the dependency.
     */
    bool GenerateDependencyGraph(fade::DynamicArray<struct Node>& out_nodes, const fade::application::CommandLineArguments& in_args);

    /**
     * Find Platform Implementation
     * 
     * Using either the platform this application was compiled for, or what was passed as command line argument to find the right implementation to build.
     */
    bool FindPlatformImplementation(const class Module* in_module, const class ModuleImplementation*& out_module_implementation) const;

    /** 
     * Get Implementations
     * 
     * Gathers an array of modules and their chosen implementations from the project configuration.
     * @param in_build_config optional string corresponding to the build config we want to use
     */
    fade::DynamicArray<ModuleImplementationPair> GetImplementations(const std::string& in_build_config) const;

private:
    fade::DynamicArray<Module> fade_modules_;

    Project project_;

    Platform platform_;
};

#endif // FADE_BUILD_HPP