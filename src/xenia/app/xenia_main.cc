/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/app/library/game_library.h"
//#include "xenia/base/debugging.h"
//#include "xenia/base/logging.h"
#include "xenia/base/cvar.h"
#include "xenia/base/main.h"
//#include "xenia/base/platform.h"
//#include "xenia/base/profiling.h"
//#include "xenia/base/threading.h"

#include "xenia/app/emulator_window.h"
#include "xenia/ui/qt/main_window.h"

#include <QApplication>
#include <QFontDatabase>
#include <QScreen>
#include <QtPlugin>

#include "discord/discord_presence.h"
#include "xenia/base/logging.h"
#include "xenia/config.h"
#include "xenia/ui/qt/loop_qt.h"
#include "xenia/app/settings/settings.h"

#if XE_PLATFORM_WIN32 && QT_STATIC
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
#endif

DEFINE_bool(show_debug_tab, true, "Show the debug tab in the Qt UI", "General");
DEFINE_string(apu, "any", "Audio system. Use: [any, nop, sdl, xaudio2]", "APU");
DEFINE_string(gpu, "any", "Graphics system. Use: [any, d3d12, vulkan, null]",
              "GPU");
DEFINE_string(hid, "any", "Input system. Use: [any, nop, sdl, winkey, xinput]",
              "HID");

DEFINE_bool(fullscreen, false, "Toggles fullscreen", "GPU");

DEFINE_path(
    storage_root, "",
    "Root path for persistent internal data storage (config, etc.), or empty "
    "to use the path preferred for the OS, such as the documents folder, or "
    "the emulator executable directory if portable.txt is present in it.",
    "Storage");

DEFINE_path(
    content_root, "",
    "Root path for guest content storage (saves, etc.), or empty to use the "
    "content folder under the storage root.",
    "Storage");

DEFINE_bool(mount_scratch, false, "Enable scratch mount", "Storage");
DEFINE_bool(mount_cache, false, "Enable cache mount", "Storage");

DEFINE_transient_path(target, "",
                      "Specifies the target .xex or .iso to execute.",
                      "General");
DEFINE_transient_bool(portable, false,
                      "Specifies if Xenia should run in portable mode.",
                      "General");

DEFINE_bool(discord, true, "Enable Discord rich presence", "General");

DECLARE_bool(debug);

namespace xe {
namespace app {

int xenia_main(const std::vector<std::string>& args) {
  /*Profiler::Initialize();
  Profiler::ThreadEnter("main");*/

  // auto emulator = std::make_unique<xe::Emulator>(L"");

  // Start Qt
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
  QCoreApplication::setApplicationName("Xenia");
  QCoreApplication::setOrganizationName(
      "Xenia Xbox 360 Emulator Research Project");
  QCoreApplication::setOrganizationDomain("https://xenia.jp");
  QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
      Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

  std::filesystem::path storage_root = cvars::storage_root;
  if (storage_root.empty()) {
    storage_root = xe::filesystem::GetExecutableFolder();
    if (!std::filesystem::exists(storage_root / "portable.txt")) {
      storage_root = xe::filesystem::GetUserFolder();
#if defined(XE_PLATFORM_WIN32) || defined(XE_PLATFORM_LINUX)
      storage_root = storage_root / "Xenia";
#else
#warning Unhandled platform for the data root.
      storage_root = storage_root / "Xenia";
#endif
    }
  }
  storage_root = std::filesystem::absolute(storage_root);
  XELOGI("Storage root: {}", xe::path_to_utf8(storage_root));

  Config::Instance().SetupConfig(storage_root);

  auto& settings = settings::Settings::Instance();
  settings.LoadSettingsItems(); // required to be called in main() as requires cvars to be loaded already

  for (const auto& set : settings.settings()) {
    XELOGI("Found settings set {}", set.title);

    for (const auto& group : set.groups) {
      XELOGI("Found settings group {0} in set {1}", group.title, set.title);
    }
  }

  if (cvars::discord) {
    discord::DiscordPresence::Initialize();
    discord::DiscordPresence::NotPlaying();
  }

  int argc = 1;
  char* argv[] = {"xenia", nullptr};
  QApplication app(argc, argv);

  // Load Fonts
  QFontDatabase fonts;
  fonts.addApplicationFont(":/resources/fonts/SegMDL2.ttf");
  fonts.addApplicationFont(":/resources/fonts/segoeui.ttf");
  fonts.addApplicationFont(":/resources/fonts/segoeuisb.ttf");
  QApplication::setFont(QFont("Segoe UI"));

  // EmulatorWindow main_wnd;
  ui::qt::QtLoop loop;
  auto main_wnd = new ui::qt::MainWindow(&loop, "Xenia Qt");
  main_wnd->Initialize();
  main_wnd->SetIcon(QIcon(":/resources/graphics/icon.ico"));
  main_wnd->Resize(1280, 720);
  main_wnd->move(QApplication::primaryScreen()->geometry().center() -
                 main_wnd->rect().center());
  loop.on_quit.AddListener([](ui::UIEvent*)
  {
    if (cvars::discord) {
      discord::DiscordPresence::Shutdown();
    }
  });
  /*
  if (FLAGS_mount_scratch) {
    auto scratch_device = std::make_unique<xe::vfs::HostPathDevice>(
        "\\SCRATCH", L"scratch", false);
    if (!scratch_device->Initialize()) {
      XELOGE("Unable to scan scratch path");
    } else {
      if (!emulator->file_system()->RegisterDevice(std::move(scratch_device))) {
        XELOGE("Unable to register scratch path");
      } else {
        emulator->file_system()->RegisterSymbolicLink("scratch:", "\\SCRATCH");
      }
    }
  }





  if (FLAGS_mount_cache) {
    auto cache0_device =
        std::make_unique<xe::vfs::HostPathDevice>("\\CACHE0", L"cache0", false);
    if (!cache0_device->Initialize()) {
      XELOGE("Unable to scan cache0 path");
    } else {
      if (!emulator->file_system()->RegisterDevice(std::move(cache0_device))) {
        XELOGE("Unable to register cache0 path");
      } else {
        emulator->file_system()->RegisterSymbolicLink("cache0:", "\\CACHE0");
      }
    }

    auto cache1_device =
        std::make_unique<xe::vfs::HostPathDevice>("\\CACHE1", L"cache1", false);
    if (!cache1_device->Initialize()) {
      XELOGE("Unable to scan cache1 path");
    } else {
      if (!emulator->file_system()->RegisterDevice(std::move(cache1_device))) {
        XELOGE("Unable to register cache1 path");
      } else {
        emulator->file_system()->RegisterSymbolicLink("cache1:", "\\CACHE1");
      }
    }
  }

  // Set a debug handler.
  // This will respond to debugging requests so we can open the debug UI.
  std::unique_ptr<xe::debug::ui::DebugWindow> debug_window;
  if (FLAGS_debug) {
    emulator->processor()->set_debug_listener_request_handler(
        [&](xe::cpu::Processor* processor) {
          if (debug_window) {
            return debug_window.get();
          }
          emulator_window->loop()->PostSynchronous([&]() {
            debug_window = xe::debug::ui::DebugWindow::Create(
                emulator.get(), emulator_window->loop());
            debug_window->window()->on_closed.AddListener(
                [&](xe::ui::UIEvent* e) {
                  emulator->processor()->set_debug_listener(nullptr);
                  emulator_window->loop()->Post(
                      [&]() { debug_window.reset(); });
                });
          });
          return debug_window.get();
        });
  }
  */

  // if (args.size() >= 2) {
  //  // Launch the path passed in args[1].
  //  main_wnd.Launch(args[1]);
  //}

  int rc = app.exec();
  loop.AwaitQuit();

  /*Profiler::Dump();
  Profiler::Shutdown();*/
  return rc;
}

}  // namespace app
}  // namespace xe

DEFINE_ENTRY_POINT("xenia", xe::app::xenia_main, "[Path to .iso/.xex]",
                   "target");
