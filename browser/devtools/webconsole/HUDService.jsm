





"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;


Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "gDevTools",
    "resource:///modules/devtools/gDevTools.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "TargetFactory",
    "resource:///modules/devtools/Target.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
    "resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DebuggerServer",
  "resource://gre/modules/devtools/dbg-server.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DebuggerClient",
  "resource://gre/modules/devtools/dbg-client.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "WebConsoleUtils",
    "resource://gre/modules/devtools/WebConsoleUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "webConsoleDefinition",
    "resource:///modules/devtools/ToolDefinitions.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
    "resource://gre/modules/commonjs/sdk/core/promise.js");

XPCOMUtils.defineLazyModuleGetter(this, "ViewHelpers",
    "resource:///modules/devtools/ViewHelpers.jsm");

const STRINGS_URI = "chrome://browser/locale/devtools/webconsole.properties";
let l10n = new WebConsoleUtils.l10n(STRINGS_URI);

const BROWSER_CONSOLE_WINDOW_FEATURES = "chrome,titlebar,toolbar,centerscreen,resizable,dialog=no";

this.EXPORTED_SYMBOLS = ["HUDService"];




function HUD_SERVICE()
{
  this.hudReferences = {};
}

HUD_SERVICE.prototype =
{
  



  hudReferences: null,

  




  get consoleUI() {
    return HeadsUpDisplayUICommands;
  },

  




  currentContext: function HS_currentContext() {
    return Services.wm.getMostRecentWindow("navigator:browser");
  },

  













  openWebConsole:
  function HS_openWebConsole(aTarget, aIframeWindow, aChromeWindow)
  {
    let hud = new WebConsole(aTarget, aIframeWindow, aChromeWindow);
    this.hudReferences[hud.hudId] = hud;
    return hud.init();
  },

  













  openBrowserConsole:
  function HS_openBrowserConsole(aTarget, aIframeWindow, aChromeWindow)
  {
    let hud = new BrowserConsole(aTarget, aIframeWindow, aChromeWindow);
    this.hudReferences[hud.hudId] = hud;
    return hud.init();
  },

  





  getHudByWindow: function HS_getHudByWindow(aContentWindow)
  {
    for each (let hud in this.hudReferences) {
      let target = hud.target;
      if (target && target.tab && target.window === aContentWindow) {
        return hud;
      }
    }
    return null;
  },

  






  getHudIdByWindow: function HS_getHudIdByWindow(aContentWindow)
  {
    let hud = this.getHudByWindow(aContentWindow);
    return hud ? hud.hudId : null;
  },

  





  getHudReferenceById: function HS_getHudReferenceById(aId)
  {
    return aId in this.hudReferences ? this.hudReferences[aId] : null;
  },

  






  lastFinishedRequestCallback: null,
};



















function WebConsole(aTarget, aIframeWindow, aChromeWindow)
{
  this.iframeWindow = aIframeWindow;
  this.chromeWindow = aChromeWindow;
  this.hudId = "hud_" + Date.now();
  this.target = aTarget;

  this.browserWindow = this.chromeWindow.top;

  let element = this.browserWindow.document.documentElement;
  if (element.getAttribute("windowtype") != "navigator:browser") {
    this.browserWindow = HUDService.currentContext();
  }
}

WebConsole.prototype = {
  iframeWindow: null,
  chromeWindow: null,
  browserWindow: null,
  hudId: null,
  target: null,
  _browserConsole: false,
  _destroyer: null,

  





  get lastFinishedRequestCallback() HUDService.lastFinishedRequestCallback,

  



  get mainPopupSet()
  {
    return this.browserWindow.document.getElementById("mainPopupSet");
  },

  



  get outputNode()
  {
    return this.ui ? this.ui.outputNode : null;
  },

  get gViewSourceUtils() this.browserWindow.gViewSourceUtils,

  





  init: function WC_init()
  {
    this.ui = new this.iframeWindow.WebConsoleFrame(this);
    return this.ui.init().then(() => this);
  },

  





  getPanelTitle: function WC_getPanelTitle()
  {
    let url = this.ui ? this.ui.contentLocation : "";
    return l10n.getFormatStr("webConsoleWindowTitleAndURL", [url]);
  },

  




  get jsterm()
  {
    return this.ui ? this.ui.jsterm : null;
  },

  



  _onClearButton: function WC__onClearButton()
  {
    if (this.target.isLocalTab) {
      this.browserWindow.DeveloperToolbar.resetErrorsCount(this.target.tab);
    }
  },

  



  setFilterState: function WC_setFilterState()
  {
    this.ui && this.ui.setFilterState.apply(this.ui, arguments);
  },

  





  openLink: function WC_openLink(aLink)
  {
    this.browserWindow.openUILinkIn(aLink, "tab");
  },

  







  viewSource: function WC_viewSource(aSourceURL, aSourceLine)
  {
    this.gViewSourceUtils.viewSource(aSourceURL, null,
                                     this.iframeWindow.document, aSourceLine);
  },

  











  viewSourceInStyleEditor:
  function WC_viewSourceInStyleEditor(aSourceURL, aSourceLine)
  {
    gDevTools.showToolbox(this.target, "styleeditor").then(function(toolbox) {
      try {
        toolbox.getCurrentPanel().selectStyleSheet(aSourceURL, aSourceLine);
      } catch(e) {
        
        this.viewSource(aSourceURL, aSourceLine);
      }
    });
  },

  









  viewSourceInDebugger:
  function WC_viewSourceInDebugger(aSourceURL, aSourceLine)
  {
    let self = this;
    let panelWin = null;
    let debuggerWasOpen = true;
    let toolbox = gDevTools.getToolbox(this.target);
    if (!toolbox) {
      self.viewSource(aSourceURL, aSourceLine);
      return;
    }

    if (!toolbox.getPanel("jsdebugger")) {
      debuggerWasOpen = false;
      let toolboxWin = toolbox.doc.defaultView;
      toolboxWin.addEventListener("Debugger:AfterSourcesAdded",
                                  function afterSourcesAdded() {
        toolboxWin.removeEventListener("Debugger:AfterSourcesAdded",
                                       afterSourcesAdded);
        loadScript();
      });
    }

    toolbox.selectTool("jsdebugger").then(function onDebuggerOpen(dbg) {
      panelWin = dbg.panelWin;
      if (debuggerWasOpen) {
        loadScript();
      }
    });

    function loadScript() {
      let debuggerView = panelWin.DebuggerView;
      if (!debuggerView.Sources.containsValue(aSourceURL)) {
        toolbox.selectTool("webconsole");
        self.viewSource(aSourceURL, aSourceLine);
        return;
      }
      if (debuggerWasOpen && debuggerView.Sources.selectedValue == aSourceURL) {
        debuggerView.editor.setCaretPosition(aSourceLine - 1);
        return;
      }

      panelWin.addEventListener("Debugger:SourceShown", onSource, false);
      debuggerView.Sources.preferredSource = aSourceURL;
    }

    function onSource(aEvent) {
      if (aEvent.detail.url != aSourceURL) {
        return;
      }
      panelWin.removeEventListener("Debugger:SourceShown", onSource, false);
      panelWin.DebuggerView.editor.setCaretPosition(aSourceLine - 1);
    }
  },

  












  getDebuggerFrames: function WC_getDebuggerFrames()
  {
    let toolbox = gDevTools.getToolbox(this.target);
    if (!toolbox) {
      return null;
    }
    let panel = toolbox.getPanel("jsdebugger");
    if (!panel) {
      return null;
    }
    let framesController = panel.panelWin.gStackFrames;
    let thread = framesController.activeThread;
    if (thread && thread.paused) {
      return {
        frames: thread.cachedFrames,
        selected: framesController.currentFrame,
      };
    }
    return null;
  },

  






  destroy: function WC_destroy()
  {
    if (this._destroyer) {
      return this._destroyer.promise;
    }

    delete HUDService.hudReferences[this.hudId];

    this._destroyer = Promise.defer();

    let popupset = this.mainPopupSet;
    let panels = popupset.querySelectorAll("panel[hudId=" + this.hudId + "]");
    for (let panel of panels) {
      panel.hidePopup();
    }

    let onDestroy = function WC_onDestroyUI() {
      try {
        let tabWindow = this.target.isLocalTab ? this.target.window : null;
        tabWindow && tabWindow.focus();
      }
      catch (ex) {
        
      }

      let id = WebConsoleUtils.supportsString(this.hudId);
      Services.obs.notifyObservers(id, "web-console-destroyed", null);
      this._destroyer.resolve(null);
    }.bind(this);

    if (this.ui) {
      this.ui.destroy().then(onDestroy);
    }
    else {
      onDestroy();
    }

    return this._destroyer.promise;
  },
};



















function BrowserConsole()
{
  WebConsole.apply(this, arguments);
}

ViewHelpers.create({ constructor: BrowserConsole, proto: WebConsole.prototype },
{
  _browserConsole: true,
  _bc_init: null,
  _bc_destroyer: null,

  $init: WebConsole.prototype.init,

  





  init: function BC_init()
  {
    if (this._bc_init) {
      return this._bc_init;
    }

    let window = this.iframeWindow;
    let onClose = () => {
      window.removeEventListener("unload", onClose);
      this.destroy();
    };
    window.addEventListener("unload", onClose);

    this._bc_init = this.$init().then((aReason) => {
      let title = this.ui.rootElement.getAttribute("browserConsoleTitle");
      this.ui.rootElement.setAttribute("title", title);
      return aReason;
    });

    return this._bc_init;
  },

  $destroy: WebConsole.prototype.destroy,

  





  destroy: function BC_destroy()
  {
    if (this._bc_destroyer) {
      return this._bc_destroyer.promise;
    }

    this._bc_destroyer = Promise.defer();

    let chromeWindow = this.chromeWindow;
    this.$destroy().then(() =>
      this.target.client.close(() => {
        HeadsUpDisplayUICommands._browserConsoleID = null;
        chromeWindow.close();
        this._bc_destroyer.resolve(null);
      }));

    return this._bc_destroyer.promise;
  },
});






var HeadsUpDisplayUICommands = {
  _browserConsoleID: null,
  _browserConsoleDefer: null,

  






  toggleHUD: function UIC_toggleHUD()
  {
    let window = HUDService.currentContext();
    let target = TargetFactory.forTab(window.gBrowser.selectedTab);
    let toolbox = gDevTools.getToolbox(target);

    return toolbox && toolbox.currentToolId == "webconsole" ?
        toolbox.destroy() :
        gDevTools.showToolbox(target, "webconsole");
  },

  






  getOpenHUD: function UIC_getOpenHUD()
  {
    let tab = HUDService.currentContext().gBrowser.selectedTab;
    if (!tab || !TargetFactory.isKnownTab(tab)) {
      return null;
    }
    let target = TargetFactory.forTab(tab);
    let toolbox = gDevTools.getToolbox(target);
    let panel = toolbox ? toolbox.getPanel("webconsole") : null;
    return panel ? panel.hud : null;
  },

  


  toggleBrowserConsole: function UIC_toggleBrowserConsole()
  {
    if (this._browserConsoleID) {
      let hud = HUDService.getHudReferenceById(this._browserConsoleID);
      return hud.destroy();
    }

    if (this._browserConsoleDefer) {
      return this._browserConsoleDefer.promise;
    }

    this._browserConsoleDefer = Promise.defer();

    function connect()
    {
      let deferred = Promise.defer();

      if (!DebuggerServer.initialized) {
        DebuggerServer.init();
        DebuggerServer.addBrowserActors();
      }

      let client = new DebuggerClient(DebuggerServer.connectPipe());
      client.connect(() =>
        client.listTabs((aResponse) => {
          
          let globals = JSON.parse(JSON.stringify(aResponse));
          delete globals.tabs;
          delete globals.selected;
          
          
          if (Object.keys(globals).length > 1) {
            deferred.resolve({ form: globals, client: client, chrome: true });
          } else {
            deferred.reject("Global console not found!");
          }
        }));

      return deferred.promise;
    }

    let target;
    function getTarget(aConnection)
    {
      let options = {
        form: aConnection.form,
        client: aConnection.client,
        chrome: true,
      };

      return TargetFactory.forRemoteTab(options);
    }

    function openWindow(aTarget)
    {
      target = aTarget;

      let deferred = Promise.defer();

      let win = Services.ww.openWindow(null, webConsoleDefinition.url, "_blank",
                                       BROWSER_CONSOLE_WINDOW_FEATURES, null);
      win.addEventListener("load", function onLoad() {
        win.removeEventListener("load", onLoad);
        deferred.resolve(win);
      });

      return deferred.promise;
    }

    connect().then(getTarget).then(openWindow).then((aWindow) =>
      HUDService.openBrowserConsole(target, aWindow, aWindow)
        .then((aBrowserConsole) => {
          this._browserConsoleID = aBrowserConsole.hudId;
          this._browserConsoleDefer.resolve(aBrowserConsole);
          this._browserConsoleDefer = null;
        }));

    return this._browserConsoleDefer.promise;
  },

  get browserConsole() {
    return HUDService.getHudReferenceById(this._browserConsoleID);
  },
};

const HUDService = new HUD_SERVICE();
