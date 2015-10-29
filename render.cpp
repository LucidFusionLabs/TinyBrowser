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

#include "lfapp/lfapp.h"
#include "lfapp/dom.h"
#include "lfapp/css.h"
#include "lfapp/flow.h"
#include "lfapp/gui.h"
#include "lfapp/ipc.h"
#include "lfapp/browser.h"
#include "crawler/html.h"
#include "crawler/document.h"

#ifdef __APPLE__
#include <sandbox.h>
#endif

namespace LFL {
DEFINE_bool(render_log, false, "Output render log");

unique_ptr<Browser> browser;
unique_ptr<BrowserController> browser_controller;
Browser::RenderLog render_log;

}; // namespace LFL
using namespace LFL;

extern "C" void LFAppCreateCB() {
  app->name = "LTerminalRenderSandbox";
  app->logfilename = StrCat(LFAppDownloadDir(), "lterm-render.txt");
  Fonts::DefaultFontEngine()->SetDefault();
  Singleton<Fonts>::Get()->default_font_engine = Singleton<IPCClientFontEngine>::Get();
  FLAGS_font_engine = "ipc_client";
}

extern "C" int main(int argc, const char *argv[]) {
  if (app->Create(argc, argv, __FILE__, LFAppCreateCB)) { app->Free(); return -1; }

  int optind = Singleton<FlagMap>::Get()->optind;
  if (optind >= argc) { fprintf(stderr, "Usage: %s [-flags] <socket-name>\n", argv[0]); return -1; }

  app->input = new Input();
  (app->assets = new Assets())->Init();

  const string socket_name = StrCat(argv[optind]);
  app->main_process = new ProcessAPIServer();
  app->main_process->OpenSocket(StrCat(argv[optind]));

  browser = unique_ptr<Browser>(new Browser(NULL, screen->Box()));
  browser->InitLayers(new LayersIPCClient());
  if (FLAGS_render_log) browser->render_log = &render_log;
  browser_controller = unique_ptr<BrowserController>(new BrowserController(browser.get()));

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
  return 0;
}
