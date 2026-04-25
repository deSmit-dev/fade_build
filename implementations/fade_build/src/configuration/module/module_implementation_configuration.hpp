#ifndef FADE_BUILD_CONFIGURATION_MODULE_IMPLEMENTATION_CONFIGURATION_HPP_
#define FADE_BUILD_CONFIGURATION_MODULE_IMPLEMENTATION_CONFIGURATION_HPP_

// Fade includes
#include "core/include/containers/dynamic_array.hpp"
#include "core/include/serialization/serialization.hpp"

// STL includes
#include <string>

struct ModuleImplementationPair
{
    // Short name of the module
    std::string module;

    // Short name of the implementation
    std::string implementation;
};

template <fade::InputArchiveType ArchiveType>
bool Serialize(ArchiveType& in_archive, ModuleImplementationPair& out_module_implementation_pair)
{
    //ARCHIVE_PARAM(in_archive, out_module_implementation_pair, module)
    //ARCHIVE_PARAM(in_archive, out_module_implementation_pair, implementation);

    return true;
}

/**
 * Fade Framework Module Implementation Configuration Entry
 * 
 * Used to match modules to implementations.
 */
struct ImplementationDescription
{
    // Name of the configuration
    std::string name;

    // Module implementation pairs
    fade::DynamicArray<ModuleImplementationPair> module_implementation_pairs;
};

template <fade::InputArchiveType ArchiveType>
bool Serialize(ArchiveType& in_archive, ImplementationDescription& out_module_implementation_configuration_entry)
{
    ARCHIVE_PARAM(in_archive, out_module_implementation_configuration_entry, name)
    ARCHIVE_PARAM(in_archive, out_module_implementation_configuration_entry, module_implementation_pairs);

    return true;
}

#endif // FADE_BUILD_CONFIGURATION_MODULE_IMPLEMENTATION_CONFIGURATION_HPP_