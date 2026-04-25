#ifndef FADE_BUILD_TYPES_HPP_
#define FADE_BUILD_TYPES_HPP_

// Fade includes
#include "core/include/type_definitions.hpp"
#include "core/include/reflection/reflect_enum.hpp"

enum class Platform : fade::uint8
{
    kUnknown,
    kLinux,
    kWindows,
    kMac,
    kAndroid
};

template <>
Platform fade::StringToEnum(const std::string& in_string)
{
    if (in_string == "linux") return Platform::kLinux;
    if (in_string == "windows") return Platform::kWindows;
    if (in_string == "mac") return Platform::kMac;
    if (in_string == "android") return Platform::kAndroid;

    return Platform::kUnknown;
}

template <>
std::string fade::EnumToString(const Platform& in_platform)
{
    switch (in_platform)
    {
        case Platform::kLinux:
            return std::string("linux");
        case Platform::kWindows:
            return std::string("windows");
        case Platform::kMac:
            return std::string("mac");
        case Platform::kAndroid:
            return std::string("android");
        case Platform::kUnknown:
            break;
    }

    return std::string("unknown");
}


#endif // FADE_BUILD_TYPES_HPP_