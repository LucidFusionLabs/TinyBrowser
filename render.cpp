/*
 * $Id$
 * Copyright (C) 2009 Lucid Fusion Labs

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "core/app/app.h"
#include "core/app/gui.h"
#include "core/app/ipc.h"
#include "core/web/browser.h"
#include "core/web/document.h"

#ifdef __APPLE__
#include <sandbox.h>
#endif

namespace LFL {
DEFINE_bool(render_log, false, "Output render log");

}; // namespace LFL
using namespace LFL;

extern "C" void MyAppCreate(int argc, const char* const* argv) {
  app = new Application(argc, argv);
  app->focused = Window::Create();
  app->name = "LBrowserRenderSandbox";
  app->log_pid = true;
  app->fonts->DefaultFontEngine()->SetDefault();
  app->fonts->default_font_engine = app->fonts->ipc_client_engine.get();
  FLAGS_font_engine = "ipc_client";
  FLAGS_max_rlimit_open_files = 1;
}

extern "C" int MyAppMain() {
  if (app->Create(__FILE__)) return -1;

  int optind = Singleton<FlagMap>::Get()->optind;
  if (optind >= app->argc) { fprintf(stderr, "Usage: %s [-flags] <socket-name>\n", app->argv[0]); return -1; }

  app->input = make_unique<Input>();
  app->net = make_unique<SocketServices>();
  app->focused->gd = CreateGraphicsDevice(app->focused, 2).release();
  (app->asset_loader = make_unique<AssetLoader>())->Init();

  const string socket_name = StrCat(app->argv[optind]);
  app->main_process = make_unique<ProcessAPIServer>();
  app->main_process->OpenSocket(StrCat(app->argv[optind]));

  unique_ptr<Browser> browser = make_unique<Browser>(nullptr, app->focused->Box());
  browser->InitLayers(make_unique<LayersIPCClient>());
  app->focused->AddInputController(make_unique<BrowserController>(browser.get()));

  Browser::RenderLog render_log;
  if (FLAGS_render_log) browser->render_log = &render_log;

#ifdef __APPLE__
  char *sandbox_error=0;
  sandbox_init(kSBXProfilePureComputation, SANDBOX_NAMED, &sandbox_error);
  INFO("render: sandbox init: ", sandbox_error ? sandbox_error : "success");
#endif

  app->main_process->browser = browser.get();
  while (app->run) {
    if (!app->main_process->HandleMessages()) break;
    if (!browser->doc.Dirty()) continue;
    browser->Render();
    if (FLAGS_render_log) { printf("Render log: %s\n", render_log.data.c_str()); render_log.Clear(); }
    app->main_process->SetDocsize(0, browser.get()->doc.height);
  }

  INFO("render: exiting");
  delete app->main_process->conn;
  return 0;
}
