#ifndef FADE_BUILD_CONFIGURATION_VERSION_HPP_
#define FADE_BUILD_CONFIGURATION_VERSION_HPP_

#include "core/include/serialization/serialization.hpp"

#include <string>
#include <cassert>

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

    /**
     * Is Compatible
     * 
     * Checks to see if @param['in_source´] is compatible with the target @param['in_target']
     * First the major versions are checked, if they do not match this function returns false as this means that source and target ar incompatible.
     * Afterwards, the minor versions are checked, if they do not match we still return true but log a warning.
     * 
     * @returns
     *  - false if the major versions do not match
     *  - true otherwise
     */
    static bool IsCompatible(const Version& in_source, const Version& in_target)
    {
        if (in_source.major != in_target.major)
        {
            fade::Log<fade::LogLevel::kError>("source major version {} is imcompatible with target major version {}", in_source.major, in_target.major);
            return false;
        }

        if (in_source.minor != in_target.minor)
        {
            fade::Log<fade::LogLevel::kWarning>("source minor version {} is not the same as target minor version {}, please use caution as they might be incompatible", in_source.minor, in_target.minor);
        }

        return true;
    };
};

template <fade::InputArchiveType ArchiveType>
bool Serialize(ArchiveType& in_archive, Version& out_version)
{
    //ARCHIVE_PARAM(in_archive, out_version, major)
    //ARCHIVE_PARAM(in_archive, out_version, minor)
    //ARCHIVE_PARAM(in_archive, out_version, revision)
    return true;
}

#endif