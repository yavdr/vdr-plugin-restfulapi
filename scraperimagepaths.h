#ifndef RESTFULAPI_SCRAPER_IMAGE_PATHS_H
#define RESTFULAPI_SCRAPER_IMAGE_PATHS_H

#include <filesystem>
#include <string>
#include <system_error>

// Shared, VDR-independent filesystem rules for scraper JSON paths and image GETs.
// An absent root is valid (there are simply no images to expose).
namespace ScraperImagePaths {
namespace fs = std::filesystem;

inline bool validRelative(const fs::path& path) {
  if (path.empty() || path.is_absolute() || path.has_root_name())
    return false;
  const std::string raw = path.generic_string();
  // The responder consumes URL paths: reject ambiguous encodings and separators.
  if (raw.find_first_of("\\?#") != std::string::npos)
    return false;
  for (unsigned char c : raw)
    if (c < 0x20 || c == 0x7f)
      return false;
  for (const auto& segment : path) {
    if (segment.empty() || segment == "." || segment == "..")
      return false;
  }
  return true;
}

inline bool decodeUrlPath(const std::string& encoded, std::string& output) {
  output.clear();
  for (size_t i = 0; i < encoded.size(); ++i) {
    unsigned char c = static_cast<unsigned char>(encoded[i]);
    if (c == '%') {
      if (i + 2 >= encoded.size())
        return false;
      auto hex = [](unsigned char d) -> int {
        if (d >= '0' && d <= '9') return d - '0';
        if (d >= 'a' && d <= 'f') return d - 'a' + 10;
        if (d >= 'A' && d <= 'F') return d - 'A' + 10;
        return -1;
      };
      const int hi = hex(encoded[i + 1]);
      const int lo = hex(encoded[i + 2]);
      if (hi < 0 || lo < 0)
        return false;
      c = static_cast<unsigned char>((hi << 4) | lo);
      i += 2;
      // Encoded path separators or controls must not change the URL routing.
      if (c == '/' || c == '\\' || c < 0x20 || c == 0x7f)
        return false;
    }
    output.push_back(static_cast<char>(c));
  }
  return validRelative(fs::path(output));
}

inline std::string relative(const std::string& image, const std::string& root) {
  if (root.empty() || image.empty())
    return "";
  const fs::path base(root);
  if (!base.is_absolute())
    return "";
  const fs::path source(image);
  // TVScraper supplies absolute image paths. Older plugins may supply paths
  // relative to their image root; preserve that behavior when safe.
  const fs::path result = source.is_absolute()
      ? source.lexically_relative(base.lexically_normal()) : source;
  if (!validRelative(result))
    return "";
  return result.generic_string();
}

inline std::string existingFile(const std::string& root,
                                const std::string& image) {
  if (root.empty() || !validRelative(fs::path(image)))
    return "";
  std::error_code ec;
  const fs::path base = fs::canonical(fs::path(root), ec);
  if (ec || !fs::is_directory(base, ec) || ec)
    return "";
  const fs::path full = fs::canonical(base / image, ec);
  if (ec || !fs::is_regular_file(full, ec) || ec)
    return "";
  const fs::path confined = full.lexically_relative(base);
  if (!validRelative(confined))
    return ""; // Also rejects symlinks resolving outside the image root.
  return full.string();
}

// Validate the parent *after* resolving symlinks. This is used only inside a
// designated private cache, and does not itself create any directories.
inline std::string writableCacheFile(const std::string& root,
                                     const std::string& image) {
  if (root.empty() || !validRelative(fs::path(image)))
    return "";
  std::error_code ec;
  const fs::path base = fs::canonical(fs::path(root), ec);
  if (ec || !fs::is_directory(base, ec) || ec)
    return "";
  const fs::path parent = fs::weakly_canonical(base / fs::path(image).parent_path(), ec);
  if (ec || (parent != base &&
             !validRelative(parent.lexically_relative(base))))
    return "";
  // Do not write through an existing symlink at the target path.
  const fs::path target = parent / fs::path(image).filename();
  if (fs::is_symlink(fs::symlink_status(target, ec)))
    return "";
  return target.string();
}
} // namespace ScraperImagePaths

#endif
