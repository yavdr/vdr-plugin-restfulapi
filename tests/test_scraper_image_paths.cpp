#include "../scraperimagepaths.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

int main() {
  char tmp[] = "/tmp/restfulapi-scraper-paths-XXXXXX";
  const char* dir = mkdtemp(tmp);
  assert(dir != nullptr);
  const fs::path root(dir);
  const fs::path media = root / "series";
  fs::create_directories(media);
  std::ofstream(media / "poster 1.jpg") << "test";
  // Test a real symlink escape to another existing regular file.
  char outsideTemplate[] = "/tmp/restfulapi-external-image-XXXXXX";
  const char* outsideDir = mkdtemp(outsideTemplate);
  assert(outsideDir != nullptr);
  const fs::path external(outsideDir);
  std::ofstream(external / "not-owned.png") << "external";
  fs::create_symlink(external / "not-owned.png", media / "escape.png");

  const std::string image = (media / "poster 1.jpg").string();
  const std::string rootStr = root.string();
  using namespace ScraperImagePaths;
  assert(relative(image, rootStr) == "series/poster 1.jpg");
  assert(relative("series/poster 1.jpg", rootStr) == "series/poster 1.jpg");
  assert(relative(image, "").empty());
  assert(relative("/elsewhere/poster.jpg", rootStr).empty());
  assert(relative(rootStr + "/../bad.jpg", rootStr).empty());
  assert(relative("../bad.jpg", rootStr).empty());
  assert(relative("", rootStr).empty());

  std::string decoded;
  assert(decodeUrlPath("series/poster%201.jpg", decoded));
  assert(decoded == "series/poster 1.jpg");
  assert(!decodeUrlPath("series/../bad.jpg", decoded));
  assert(!decodeUrlPath("series/%2e%2e/bad.jpg", decoded));
  assert(!decodeUrlPath("series/%2fbad.jpg", decoded));
  assert(!decodeUrlPath("series/%00bad.jpg", decoded));
  assert(!decodeUrlPath("series/poster%xy.jpg", decoded));
  assert(!decodeUrlPath("/etc/passwd", decoded));
  assert(!decodeUrlPath("series\\..\\bad.jpg", decoded));

  assert(existingFile(rootStr, "series/poster 1.jpg") ==
         fs::canonical(media / "poster 1.jpg").string());
  assert(existingFile(rootStr, "../bad.jpg").empty());
  assert(existingFile(rootStr, "series/escape.png").empty());
  assert(existingFile("", "series/poster 1.jpg").empty());
  assert(existingFile(rootStr, "missing.jpg").empty());

  assert(!writableCacheFile(rootStr, "new.jpg").empty());
  assert(!writableCacheFile(rootStr, "sizes/200/new.jpg").empty());
  assert(writableCacheFile(rootStr, "../new.jpg").empty());
  fs::create_symlink(external / "not-owned.png", root / "symlink.png");
  assert(writableCacheFile(rootStr, "symlink.png").empty());

  fs::remove_all(root);
  fs::remove_all(external);
  std::cout << "PASS scraper image paths, empty roots, escaping and confinement\n";
}
