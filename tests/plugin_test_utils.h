#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>

namespace vivid_sequencers::test {

#if defined(_WIN32)
inline constexpr const char* kPluginSuffix = ".dll";
#elif defined(__APPLE__)
inline constexpr const char* kPluginSuffix = ".dylib";
#else
inline constexpr const char* kPluginSuffix = ".so";
#endif

inline std::filesystem::path plugin_path(const std::filesystem::path& dir,
                                         const char* stem) {
    return dir / (std::string(stem) + kPluginSuffix);
}

inline bool copy_plugin_if_exists(const std::filesystem::path& src_dir,
                                  const std::filesystem::path& dst_dir,
                                  const char* stem,
                                  std::error_code& ec) {
    ec.clear();
    std::filesystem::copy_file(
        plugin_path(src_dir, stem),
        plugin_path(dst_dir, stem),
        std::filesystem::copy_options::overwrite_existing,
        ec);
    return !ec;
}

inline void copy_plugin_or_throw(const std::filesystem::path& src_dir,
                                 const std::filesystem::path& dst_dir,
                                 const char* stem) {
    std::error_code ec;
    if (!copy_plugin_if_exists(src_dir, dst_dir, stem, ec)) {
        throw std::runtime_error("failed to copy plugin '" + std::string(stem)
                                 + "': " + ec.message());
    }
}

} // namespace vivid_sequencers::test
