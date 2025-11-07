#include "processor-loader.h"

#include <algorithm>
#include <dlfcn.h>
#include <filesystem>
#include <mutex>
#include <unordered_map>

namespace limb {

namespace fs = std::filesystem;

typedef limb::ImageProcessor *(*create_fn_t)();
typedef void (*destroy_fn_t)(limb::ImageProcessor *);
typedef const char *(*name_fn_t)();

struct ProcessorModule {
  void *m_handle;
  create_fn_t create_fn;
  destroy_fn_t destroy_fn;
  name_fn_t name_fn;
};

class ProcessorLoader::impl {
public:
  impl() {

    constexpr char defaultLoadDir[] = "processors";
    m_loadDirectories.push_back(defaultLoadDir);
    loadDirectories();
  }

  impl(const std::vector<std::string> &additionalDirectories) {
    for (const auto &newDir : additionalDirectories) {
      if (!fs::exists(newDir) || !fs::is_directory(newDir))
        continue;

      const auto it = std::find_if(m_loadDirectories.begin(), m_loadDirectories.end(),
                                   [&newDir](const std::string_view additionalDirectory) {
                                     return fs::equivalent(newDir, additionalDirectory);
                                   });

      if (it == m_loadDirectories.end()) {
        m_loadDirectories.emplace_back(newDir);
      }
    }
    loadDirectories();
  }

  ~impl() { unload(); }

  liret status() { return processorCount() > 0 ? liret::kOk : liret::kAborted; }

  liret addLoadDir(const std::string &dirPath) {
    if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
      return liret::kInvalidInput;
    }

    const auto it = std::find_if(m_loadDirectories.begin(), m_loadDirectories.end(),
                                 [&dirPath](const std::string_view additionalDirectory) {
                                   return fs::equivalent(dirPath, additionalDirectory);
                                 });

    if (it != m_loadDirectories.end()) {
      return liret::kAlreadyExists;
    }

    m_loadDirectories.emplace_back(dirPath);

    return liret::kOk;
  }

  void removeLoadDir(size_t index) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (index >= m_loadDirectories.size()) {
      return;
    }
    m_loadDirectories.erase(m_loadDirectories.begin() + index);
  }

  std::string processorName(size_t index) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (index >= m_modules.size()) {
      return std::string();
    }
    auto name = m_modules[index].name_fn();
    return name ? std::string(name) : std::string();
  }

  ImageProcessor *allocateProcessor(size_t index) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (index < m_modules.size()) {
      ImageProcessor *ip = m_modules[index].create_fn();
      if (!ip) {
        return nullptr;
      }
      m_processors[ip] = index;
      return ip;
    }

    return nullptr;
  }

  void destroyProcessor(ImageProcessor *processor) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_processors.find(processor);

    if (it == m_processors.end())
      return;

    m_modules[it->second].destroy_fn(processor);
    m_processors.erase(it);
  }

  size_t processorCount() { return m_modules.size(); }
  size_t dirCount() { return m_loadDirectories.size() + 1; }

  void loadModules(const fs::path &directory) {
    constexpr char fileExtantion[] = ".so";

    if (!fs::is_directory(directory)) {
      return;
    }

    for (const auto &file : fs::directory_iterator(directory)) {
      std::string filename = file.path().filename().string();
      if (!file.is_regular_file() || filename.find(fileExtantion) == std::string::npos)
        continue;

      std::string fullPath = file.path().c_str();
      void *handle = dlopen(fullPath.c_str(), RTLD_NOW | RTLD_LOCAL);

      // TODO: consider display a warning
      if (!handle)
        continue;

      create_fn_t create_fn = reinterpret_cast<create_fn_t>(dlsym(handle, "createProcessor"));
      destroy_fn_t destroy_fn = reinterpret_cast<destroy_fn_t>(dlsym(handle, "destroyProcessor"));
      name_fn_t name_fn = reinterpret_cast<name_fn_t>(dlsym(handle, "processorName"));

      // TODO: consider display a warning
      if (!create_fn || !destroy_fn || !name_fn) {
        dlclose(handle);
        continue;
      }

      std::lock_guard<std::mutex> lock(m_mutex);
      m_modules.emplace_back(handle, create_fn, destroy_fn, name_fn);
    }
  }

  void loadDirectories() {
    for (const std::string_view additionalPath : m_loadDirectories) {
      if (!fs::exists(additionalPath) || !fs::is_directory(additionalPath))
        continue;

      fs::path additional(additionalPath);

      for (const auto &entry : fs::directory_iterator(additional)) {
        if (entry.is_directory()) {
          loadModules(entry);
        }
      }
    }
  }

  void unload() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto &module : m_modules) {
      if (module.m_handle) {
        dlclose(module.m_handle);
      }
    }
    m_modules.clear();
  }

private:
  std::vector<std::string> m_loadDirectories;
  std::unordered_map<ImageProcessor *, size_t> m_processors;
  std::vector<ProcessorModule> m_modules;
  std::mutex m_mutex;
};

liret ProcessorLoader::status() { return pImpl->status(); }

liret ProcessorLoader::reload() {
  pImpl->unload();
  pImpl->loadDirectories();
  return status();
}

liret ProcessorLoader::addLoadDir(const std::string &dirPath) {
  liret ret = pImpl->addLoadDir(dirPath);
  if (ret != liret::kOk)
    return ret;
  return reload();
}
liret ProcessorLoader::removeLoadDir(size_t index) {
  pImpl->removeLoadDir(index);
  return reload();
}

std::string ProcessorLoader::processorName(size_t index) { return pImpl->processorName(index); }
ImageProcessor *ProcessorLoader::allocateProcessor(size_t index) { return pImpl->allocateProcessor(index); }
void ProcessorLoader::destroyProcessor(ImageProcessor *processor) { return pImpl->destroyProcessor(processor); }

size_t ProcessorLoader::processorCount() { return pImpl->processorCount(); }
size_t ProcessorLoader::dirCount() { return pImpl->dirCount(); }

ProcessorLoader::ProcessorLoader() = default;
ProcessorLoader::ProcessorLoader(const std::vector<std::string> &additionalDirectories)
    : pImpl{std::make_unique<impl>(additionalDirectories)} {}
ProcessorLoader::~ProcessorLoader() = default;

} // namespace limb