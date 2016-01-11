/*
 * $Id: browser.cpp 1336 2014-12-08 09:29:59Z justin $
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
#include "web/html.h"
#include "web/document.h"

namespace LFL {
DEFINE_string(url, "http://news.google.com/", "Url to open");
DEFINE_int(width, 1040, "browser width");
DEFINE_int(height, 768, "browser height");
DEFINE_bool(render_log, false, "Output render log");
DEFINE_bool(render_sandbox, true, "Render in sandbox process");
extern FlagOfType<bool> FLAGS_lfapp_network_;

BindMap *binds;
Browser::RenderLog render_log;

struct JavaScriptConsole : public Console {
  Browser *browser;
  JavaScriptConsole(LFL::Window *W, Browser *B) : Console(W), browser(B) { bottom_or_top = 1; write_timestamp = 0; color = Color(Color::black, 0.9); }
  virtual void Run(const string &in) override {
    if (app->render_process) app->render_process->ExecuteScript(in, bind(&JavaScriptConsole::ExecuteResponseCB, this, _1));
    else { string ret = browser->doc.js_context->Execute(in); if (!ret.empty()) Write(ret); }
  }
  void ExecuteResponseCB(const string &res) { if (!res.empty()) Write(res); }
};

struct MyBrowserWindow : public GUI {
  Font *menu_atlas;
  Box win, topbar, addressbar;
  Widget::Button back, forward, refresh;
  TextGUI address_box;
  Browser *lfl_browser=0;
  BrowserInterface *browser=0;
  BrowserInterface *qt_browser=0, *berkelium_browser=0;
  BrowserController *controller=0;

  MyBrowserWindow(LFL::Window *W) : GUI(W), menu_atlas(Fonts::Get("MenuAtlas", "", 0, Color::white, Color::clear, 0)),
  back   (this, &menu_atlas->FindGlyph(6)->tex, 0, "", MouseController::CB([&](){ browser->BackButton(); })),
  forward(this, &menu_atlas->FindGlyph(7)->tex, 0, "", MouseController::CB([&](){ browser->ForwardButton(); })),
  refresh(this, &menu_atlas->FindGlyph(8)->tex, 0, "", MouseController::CB([&](){ browser->RefreshButton(); })),
  address_box(W, Fonts::Get(FLAGS_default_font, "", 12, Color::black, Color::white)) {
    address_box.bg_color = &Color::white;
    address_box.SetToggleKey(0, true);
    address_box.cmd_prefix.clear();
    address_box.deactivate_on_enter = true;
    address_box.runcb = [=](const string &t){ Open(t); };
    Activate();
  }

  void Open(const string &url) {
    if (app->network_thread) app->network_thread->Write(new Callback([=]() { browser->Open(url); }));
    else                                                                   { browser->Open(url); }
  }

  void Init() {
    InitLayout();
#if LFL_QT
    if (!browser) browser = qt_browser = CreateQTWebKitBrowser(this, win.w, win.h);
#endif
#ifdef LFL_BERKELIUM
    if (!browser) browser = berkelium_browser = CreateBerkeliumBrowser(this, win.w, win.h);
#endif
    if (!browser) {
      browser = lfl_browser = new Browser(this, win);
      lfl_browser->doc.js_console = unique_ptr<Console>(new JavaScriptConsole(screen, lfl_browser));
      lfl_browser->doc.js_console->animating_cb = bind(&MyBrowserWindow::UpdateTargetFPS, this);
      if (app->render_process) lfl_browser->InitLayers(new LayersIPCServer());
      else                     lfl_browser->InitLayers(new Layers());
      if (FLAGS_render_log)    lfl_browser->render_log = &render_log;
      lfl_browser->url_cb = [&](const string &x){ address_box.AssignInput(x); };
    }
    if (!controller) controller = new BrowserController(browser);
    if (screen->lfapp_console) screen->lfapp_console->animating_cb = bind(&MyBrowserWindow::UpdateTargetFPS, this);
    Layout();
  }

  void InitLayout() {
    box = win = screen->Box();
    win.SetPosition(point(0, -win.h));
    topbar = win;
    topbar.h = 16;
    topbar.y = win.y + win.h - topbar.h;
    win.h = max(0, win.h - topbar.h);
  }

  void Layout() {
    Reset();
    InitLayout();
    addressbar = topbar;
    MinusPlus(&addressbar.w, &addressbar.x, 16*3 + 20);
    child_box.PushBack(topbar, Drawable::Attr(0, &Color::grey70), Singleton<BoxFilled>::Get());

    Flow flow(&box, 0, &child_box);
    flow.cur_attr.font = menu_atlas;
    back   .Layout(&flow, point(16, 16)); flow.p.x += 5;
    forward.Layout(&flow, point(16, 16)); flow.p.x += 5;
    refresh.Layout(&flow, point(16, 16));
    if (lfl_browser) lfl_browser->Layout(win);
    refresh.AddClickBox(addressbar, MouseController::CB([&](){ address_box.Activate(); }));
  }

  void Draw() {
    GUI::Draw();
    browser->Draw(box);
    address_box.Draw(addressbar + box.TopLeft());
    if (lfl_browser) {
      lfl_browser->UpdateScrollbar();
      if (lfl_browser->doc.js_console) lfl_browser->doc.js_console->Draw();
    }
  }

  void UpdateTargetFPS() { app->scheduler.SetAnimating((screen->lfapp_console && screen->lfapp_console->animating) || 
                                                       (lfl_browser && lfl_browser->doc.js_console->animating)); }
};

int Frame(LFL::Window *W, unsigned clicks, int flag) {
  MyBrowserWindow *bw = (MyBrowserWindow*)W->user1;
  bw->Draw();
  screen->DrawDialogs();
  if (FLAGS_render_log && !app->render_process) { printf("Render log: %s\n", render_log.data.c_str()); render_log.Clear(); }
  return 0;
}

void MyJavaScriptConsole() {
  MyBrowserWindow *tw = (MyBrowserWindow*)screen->user1;
  if (tw->lfl_browser && tw->lfl_browser->doc.js_console) tw->lfl_browser->doc.js_console->ToggleActive();
}

void MyWindowInitCB(LFL::Window *W) {
  W->width = FLAGS_width;
  W->height = FLAGS_height;
  W->caption = "Browser";
  W->binds = binds;
  W->frame_cb = Frame;
  if (app->initialized) W->user1 = new MyBrowserWindow(W);
}

}; // namespace LFL
using namespace LFL;

extern "C" void LFAppCreateCB() {
  app->name = "LBrowser";
  app->logfilename = StrCat(LFAppDownloadDir(), "browser.txt");
  binds = new BindMap();
  MyWindowInitCB(screen);
  FLAGS_lfapp_video = FLAGS_lfapp_input = FLAGS_max_rlimit_open_files = 1;
}

extern "C" int main(int argc, const char *argv[]) {
  if (app->Create(argc, argv, __FILE__, LFAppCreateCB)) { app->Free(); return -1; }
  if (FLAGS_font_engine == "freetype") { DejaVuSansFreetype::SetDefault(); DejaVuSansFreetype::Load(); }
  if (app->Init()) { app->Free(); return -1; }
  app->scheduler.AddWaitForeverKeyboard();
  app->scheduler.AddWaitForeverMouse();

  app->network = new Network();
#if !defined(LFL_MOBILE)
  if (FLAGS_render_sandbox) {
    vector<string> arg;
    if (FLAGS_render_log) { arg.push_back("-render_log"); arg.push_back("1"); }
    app->render_process = new ProcessAPIClient();
    app->render_process->StartServerProcess(StrCat(app->bindir, "lbrowser-render-sandbox", LocalFile::ExecutableSuffix), arg);
    CHECK(app->CreateNetworkThread(false, false));
    app->network->select_time = Seconds(1).count();
  }
#else
  if (0) {}
#endif
  else app->LoadModule(app->network);

  binds->Add(Bind('6', Key::Modifier::Cmd, Bind::CB(bind([&](){ app->shell.console(vector<string>()); }))));
  binds->Add(Bind('7', Key::Modifier::Cmd, Bind::CB(bind(&MyJavaScriptConsole))));

  MyBrowserWindow *bw = new MyBrowserWindow(screen);
  screen->user1 = bw;
  bw->Init();
  if (!FLAGS_url.empty()) bw->Open(FLAGS_url);
  if (app->render_process) {
    app->render_process->browser = bw->lfl_browser;
    app->network_thread->thread->Start();
    bw->lfl_browser->SetViewport(bw->lfl_browser->viewport.w, bw->lfl_browser->viewport.h);
  }

  return app->Main();
}
