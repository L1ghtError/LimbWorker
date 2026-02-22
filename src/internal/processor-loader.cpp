#ifdef _WIN32
#include <windows.h>
typedef HMODULE LibHandle;
#else
#include <dlfcn.h>
typedef void *LibHandle;
#endif

#include "processor-loader.h"

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <unordered_map>

namespace {

typedef limb::ProcessorModule *(*getModule_fn_t)();

constexpr char defaultLoadDir[] = "processors";

LibHandle loadLibrary(const char *path) {
#ifdef _WIN32
  HMODULE handle = LoadLibraryA(path);
#else
  void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);

#endif
  return handle;
}

LibHandle loadLibraryWithLocalOrigin(const char *path) {
#ifdef _WIN32
  HMODULE handle = LoadLibraryExA(path, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
#else
  void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);

#endif
  return handle;
}

LibHandle loadLibraryUnresolved(const char *path) {
#ifdef _WIN32
  HMODULE handle = LoadLibraryExA(path, NULL, DONT_RESOLVE_DLL_REFERENCES);
#else
  // TODO: verify is RTLD_LAZY is enough for preventing symbol resolution
  void *handle = dlopen(path, RTLD_LAZY | RTLD_LOCAL | RTLD_NOLOAD);

#endif
  return handle;
}

void unloadLibrary(LibHandle handle) {
#ifdef _WIN32
  if (handle)
    FreeLibrary(handle);
#else
  if (handle)
    dlclose(handle);
#endif
}

std::string getLastDLError() {
#ifdef _WIN32
  DWORD errorCode = GetLastError();
  char buffer[512];
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errorCode, 0, buffer,
                 sizeof(buffer), nullptr);
  return buffer;
#else
  const char *err = dlerror();
  return err ? err : "unknown error";
#endif
}

const char libEXT[] =
#ifdef _WIN32
    ".dll";
#elif __APPLE__
    ".dylib";
#else
    ".so";
#endif

void *getFunction(LibHandle handle, const char *name) {
#ifdef _WIN32
  return reinterpret_cast<void *>(GetProcAddress(handle, name));
#else
  return dlsym(handle, name);
#endif
}

bool verifyModules(const char *path) {
  LibHandle handle = loadLibraryUnresolved(path);
  if (!handle) {
    return false;
  }

  getModule_fn_t getModule_fn = reinterpret_cast<getModule_fn_t>(getFunction(handle, "GetProcessorModule"));

  bool success = true;
  if (!getModule_fn) {
    success = false;
  }

  unloadLibrary(handle);
  return success;
}

} // namespace

namespace limb {

namespace fs = std::filesystem;

struct LoadedModule {
  LibHandle m_handle;
  limb::ProcessorModule *procModule;
};

class ProcessorLoader::impl {
public:
  impl() {

    m_loadDirectories.push_back(defaultLoadDir);
    loadDirectories();
  }

  explicit impl(const std::vector<std::string> &additionalDirectories) {
    m_loadDirectories.push_back(defaultLoadDir);

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

  liret status() const { return processorCount() > 0 ? liret::kOk : liret::kAborted; }

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

  std::string_view processorName(size_t index) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (index >= m_modules.size()) {
      return {};
    }

    return m_modules[index].procModule->name();
  }

  ProcessorContainer *allocateContainer(size_t index) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (index < m_modules.size()) {
      ProcessorContainer *container = m_modules[index].procModule->allocateContainer();
      if (!container) {
        return nullptr;
      }
      m_processors[container] = index;
      return container;
    }

    return nullptr;
  }

  void destroyContainer(ProcessorContainer *container) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_processors.find(container);

    if (it == m_processors.end())
      return;

    m_modules[it->second].procModule->deallocateContainer(container);
    m_processors.erase(it);
  }

  size_t processorCount() const { return m_modules.size(); }
  size_t dirCount() const { return m_loadDirectories.size() + 1; }

  void loadModules(const fs::path &directory) {
    if (!fs::is_directory(directory)) {
      return;
    }

    for (const auto &file : fs::directory_iterator(directory)) {
      std::string filename = file.path().filename().string();
      if (!file.is_regular_file() || filename.find(libEXT) == std::string::npos)
        continue;

      std::string fullPath = fs::absolute(file.path()).string();
      if (!verifyModules(fullPath.c_str())) {
        continue;
      }

      LibHandle handle = loadLibraryWithLocalOrigin(fullPath.c_str());

      // TODO: consider display a warning
      if (!handle) {
        // Try loading and resolving systemwide
        handle = loadLibrary((fullPath).c_str());
        if (!handle) {
          continue;
        }
      }

      getModule_fn_t getModule_fn = reinterpret_cast<getModule_fn_t>(getFunction(handle, "GetProcessorModule"));

      // TODO: consider display a warning
      if (!getModule_fn) {
        unloadLibrary(handle);
        continue;
      }

      std::lock_guard<std::mutex> lock(m_mutex);
      m_modules.emplace_back(handle, getModule_fn());
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
        unloadLibrary(module.m_handle);
      }
    }
    m_modules.clear();
  }

private:
  std::vector<std::string> m_loadDirectories;
  std::unordered_map<ProcessorContainer *, size_t> m_processors;
  std::vector<LoadedModule> m_modules;
  std::mutex m_mutex;
};

liret ProcessorLoader::status() const { return pImpl->status(); }

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

std::string_view ProcessorLoader::processorName(size_t index) { return pImpl->processorName(index); }
ProcessorContainer *ProcessorLoader::allocateContainer(size_t index) { return pImpl->allocateContainer(index); }
void ProcessorLoader::destroyContainer(ProcessorContainer *container) { return pImpl->destroyContainer(container); }

size_t ProcessorLoader::processorCount() const { return pImpl->processorCount(); }
size_t ProcessorLoader::dirCount() const { return pImpl->dirCount(); }

ProcessorLoader::ProcessorLoader() : pImpl{std::make_unique<impl>()} {};
ProcessorLoader::ProcessorLoader(const std::vector<std::string> &additionalDirectories)
    : pImpl{std::make_unique<impl>(additionalDirectories)} {}
ProcessorLoader::ProcessorLoader(ProcessorLoader &&pl) : pImpl{std::move(pl.pImpl)} {}
ProcessorLoader::~ProcessorLoader() = default;

} // namespace limb