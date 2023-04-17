#include "engine.h"

#include "config.h"
#include "init.h" // This includes is needed on mac so that the global variable is created
#include "interactor_impl.h"
#include "loader_impl.h"
#include "log.h"
#include "window_impl.h"

#include "factory.h"

#include "vtkF3DConfigure.h"
#include "vtkF3DNoRenderWindow.h"

#include <vtkVersion.h>

#include <vtksys/DynamicLoader.hxx>
#include <vtksys/Encoding.hxx>
#include <vtksys/SystemTools.hxx>

namespace f3d
{
class engine::internals
{
public:
  std::unique_ptr<options> Options;
  std::unique_ptr<detail::window_impl> Window;
  std::unique_ptr<detail::loader_impl> Loader;
  std::unique_ptr<detail::interactor_impl> Interactor;
};

//----------------------------------------------------------------------------
engine::engine(window::Type windowType)
  : Internals(new engine::internals)
{
  // build default cache path
#if defined(_WIN32)
  std::string cachePath = vtksys::SystemTools::GetEnv("LOCALAPPDATA");
  cachePath = cachePath + "/f3d";
#elif defined(__APPLE__)
  std::string cachePath = vtksys::SystemTools::GetEnv("HOME");
  cachePath += "/Library/Caches/f3d";
#elif defined(__ANDROID__)
  std::string cachePath = ""; // no default
#elif defined(__unix__)
  std::string cachePath = vtksys::SystemTools::GetEnv("HOME");
  cachePath += "/.cache/f3d";
#else
#error "Unsupported platform"
#endif

  this->Internals->Options = std::make_unique<options>();

  this->Internals->Window =
    std::make_unique<detail::window_impl>(*this->Internals->Options, windowType);
  this->Internals->Window->SetCachePath(cachePath);

  this->Internals->Loader =
    std::make_unique<detail::loader_impl>(*this->Internals->Options, *this->Internals->Window);

  // Do not create an interactor for NONE or EXTERNAL
  if (windowType != window::Type::NONE && windowType != window::Type::EXTERNAL)
  {
    this->Internals->Interactor = std::make_unique<detail::interactor_impl>(
      *this->Internals->Options, *this->Internals->Window, *this->Internals->Loader);
  }
}

//----------------------------------------------------------------------------
engine::~engine()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
engine& engine::setOptions(const options& opt)
{
  *this->Internals->Options = opt;
  return *this;
}

//----------------------------------------------------------------------------
engine& engine::setOptions(options&& opt)
{
  *this->Internals->Options = std::move(opt);
  return *this;
}

//----------------------------------------------------------------------------
options& engine::getOptions()
{
  return *this->Internals->Options;
}

//----------------------------------------------------------------------------
window& engine::getWindow()
{
  if (this->Internals->Window->getType() == window::Type::NONE)
  {
    throw engine::no_window_exception("No window with this engine");
  }
  return *this->Internals->Window;
}

//----------------------------------------------------------------------------
loader& engine::getLoader()
{
  return *this->Internals->Loader;
}

//----------------------------------------------------------------------------
interactor& engine::getInteractor()
{
  if (!this->Internals->Interactor)
  {
    throw engine::no_interactor_exception("No interactor with this engine");
  }
  return *this->Internals->Interactor;
}

//----------------------------------------------------------------------------
void engine::loadPlugin(const std::string& pathOrName)
{
  std::string pluginOrigin = "static";

  // check if the plugin is a known static plugin
  factory* factory = factory::instance();
  factory::plugin_initializer_t init_plugin = factory->getStaticInitializer(pathOrName);

  if (init_plugin == nullptr)
  {
    vtksys::DynamicLoader::LibraryHandle handle = nullptr;

    std::string fullPath = vtksys::SystemTools::CollapseFullPath(pathOrName);
    if (vtksys::SystemTools::FileExists(fullPath))
    {
      // plugin provided as full path
      log::debug("Trying to load plugin from: \"", fullPath, "\"");
      handle = vtksys::DynamicLoader::OpenLibrary(fullPath);

      if (!handle)
      {
        throw engine::plugin_exception(
          "Cannot open the library \"" + fullPath + "\": " + vtksys::DynamicLoader::LastError());
      }
      else
      {
        pluginOrigin = fullPath;
      }
    }
    else
    {
      std::vector<std::string> searchPaths;

      std::string envValue;
      if (vtksys::SystemTools::GetEnv("F3D_PLUGINS_PATH", envValue))
      {
        // split path with OS separator (':' on Linux/macOS and ';' on Windows)
#ifdef _WIN32
        char sep = ';';
#else
        char sep = ':';
#endif
        vtksys::SystemTools::Split(envValue, searchPaths, sep);
      }

      // On Windows, search plugins in the same folder than f3d.exe too by default
#ifdef _WIN32
      std::vector<wchar_t> pathBuf(40000);
      if (GetModuleFileNameW(
            GetModuleHandle(nullptr), pathBuf.data(), static_cast<DWORD>(pathBuf.size())))
      {
        std::string progPath =
          vtksys::SystemTools::GetProgramPath(vtksys::Encoding::ToNarrow(pathBuf.data()));
        searchPaths.push_back(progPath);
      }
#endif

      // construct the library file name from the plugin name
      std::string libName = vtksys::DynamicLoader::LibPrefix();
      libName += "f3d-plugin-";
      libName += pathOrName;
      libName += vtksys::DynamicLoader::LibExtension();

      // try search paths
      for (const std::string& path : searchPaths)
      {
        std::string tryPath = path + '/' + libName;
        tryPath = vtksys::SystemTools::ConvertToOutputPath(tryPath);

        log::debug("Trying to load plugin from: \"", tryPath, "\"");
        handle = vtksys::DynamicLoader::OpenLibrary(tryPath);

        if (handle)
        {
          // plugin is found and loaded
          pluginOrigin = tryPath;
          break;
        }
      }

      if (!handle)
      {
        // Rely on internal system (e.g. LD_LIBRARY_PATH on Linux) by giving only the file name
        log::debug("Trying to load plugin relying on internal system: ", libName);
        handle = vtksys::DynamicLoader::OpenLibrary(libName);
        if (!handle)
        {
          throw engine::plugin_exception("Cannot open the library \"" + pathOrName +
            "\": " + vtksys::DynamicLoader::LastError());
        }
        else
        {
          pluginOrigin = "system";
        }
      }
    }

    init_plugin = reinterpret_cast<factory::plugin_initializer_t>(
      vtksys::DynamicLoader::GetSymbolAddress(handle, "init_plugin"));
    if (init_plugin == nullptr)
    {
      throw engine::plugin_exception("Cannot find init_plugin symbol in library \"" + pathOrName +
        "\": " + vtksys::DynamicLoader::LastError());
    }
  }

  plugin* plug = init_plugin();
  plug->setOrigin(pluginOrigin);
  factory->load(plug);
  log::debug("Loaded plugin ",plug->getName()," from: \"", plug->getOrigin(),"\"");
}

//----------------------------------------------------------------------------
void engine::autoloadPlugins()
{
  factory::instance()->autoload();
}

//----------------------------------------------------------------------------
engine::libInformation engine::getLibInfo()
{
  libInformation libInfo;
  libInfo.Version = detail::LibVersion;
  libInfo.BuildDate = detail::LibBuildDate;
  libInfo.BuildSystem = detail::LibBuildSystem;
  libInfo.Compiler = detail::LibCompiler;

  std::string tmp;

#if F3D_MODULE_RAYTRACING
  tmp = "ON";
#else
  tmp = "OFF";
#endif
  libInfo.RaytracingModule = tmp;

#if F3D_MODULE_EXTERNAL_RENDERING
  tmp = "ON";
#else
  tmp = "OFF";
#endif
  libInfo.ExternalRenderingModule = tmp;

  libInfo.VTKVersion = std::string(VTK_VERSION) + std::string(" (build ") +
    std::to_string(VTK_BUILD_VERSION) + std::string(")");

  libInfo.PreviousCopyright = "Copyright (C) 2019-2021 Kitware SAS";
  libInfo.Copyright = "Copyright (C) 2021-2022 Michael Migliore, Mathieu Westphal";
  libInfo.License = "BSD-3-Clause";
  libInfo.Authors = "Michael Migliore, Mathieu Westphal and Joachim Pouderoux";

  return libInfo;
}

//----------------------------------------------------------------------------
std::vector<engine::readerInformation> engine::getReadersInfo()
{
  std::vector<readerInformation> readersInfo;
  const auto& plugins = factory::instance()->getPlugins();
  for (const auto& plugin : plugins)
  {
    for (const auto& reader : plugin->getReaders())
    {
      readerInformation info;
      info.PluginName = plugin->getName();
      info.Name = reader->getName();
      info.Description = reader->getLongDescription();
      info.Extensions = reader->getExtensions();
      info.MimeTypes = reader->getMimeTypes();
      info.HasSceneReader = reader->hasSceneReader();
      info.HasGeometryReader = reader->hasGeometryReader();
      readersInfo.push_back(info);
    }
  }
  return readersInfo;
}

//----------------------------------------------------------------------------
void engine::setCachePath(const std::string& cachePath)
{
  this->Internals->Window->SetCachePath(cachePath);
}

//----------------------------------------------------------------------------
engine::no_window_exception::no_window_exception(const std::string& what)
  : exception(what)
{
}

//----------------------------------------------------------------------------
engine::no_interactor_exception::no_interactor_exception(const std::string& what)
  : exception(what)
{
}

//----------------------------------------------------------------------------
engine::plugin_exception::plugin_exception(const std::string& what)
  : exception(what)
{
}

}
