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
};

template <fade::InputArchiveType ArchiveType>
bool Serialize(ArchiveType& in_archive, Version& out_version)
{
    ARCHIVE_PARAM(in_archive, out_version, major)
    ARCHIVE_PARAM(in_archive, out_version, minor)
    ARCHIVE_PARAM(in_archive, out_version, revision)
    return true;
}

#endif