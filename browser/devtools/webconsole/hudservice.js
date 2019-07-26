





"use strict";

const {Cc, Ci, Cu} = require("chrome");

let WebConsoleUtils = require("devtools/toolkit/webconsole/utils").Utils;
let Heritage = require("sdk/core/heritage");

loader.lazyGetter(this, "promise", () => require("sdk/core/promise"));
loader.lazyGetter(this, "Telemetry", () => require("devtools/shared/telemetry"));
loader.lazyGetter(this, "WebConsoleFrame", () => require("devtools/webconsole/webconsole").WebConsoleFrame);
loader.lazyImporter(this, "gDevTools", "resource:///modules/devtools/gDevTools.jsm");
loader.lazyImporter(this, "devtools", "resource://gre/modules/devtools/Loader.jsm");
loader.lazyImporter(this, "Services", "resource://gre/modules/Services.jsm");
loader.lazyImporter(this, "DebuggerServer", "resource://gre/modules/devtools/dbg-server.jsm");
loader.lazyImporter(this, "DebuggerClient", "resource://gre/modules/devtools/dbg-client.jsm");

const STRINGS_URI = "chrome://browser/locale/devtools/webconsole.properties";
let l10n = new WebConsoleUtils.l10n(STRINGS_URI);

const BROWSER_CONSOLE_WINDOW_FEATURES = "chrome,titlebar,toolbar,centerscreen,resizable,dialog=no";


const BROWSER_CONSOLE_FILTER_PREFS_PREFIX = "devtools.browserconsole.filter.";




function HUD_SERVICE()
{
  this.consoles = new Map();
  this.lastFinishedRequest = { callback: null };
}

HUD_SERVICE.prototype =
{
  _browserConsoleID: null,
  _browserConsoleDefer: null,

  



  consoles: null,

  








  lastFinishedRequest: null,

  




  currentContext: function HS_currentContext() {
    return Services.wm.getMostRecentWindow("navigator:browser");
  },

  













  openWebConsole:
  function HS_openWebConsole(aTarget, aIframeWindow, aChromeWindow)
  {
    let hud = new WebConsole(aTarget, aIframeWindow, aChromeWindow);
    this.consoles.set(hud.hudId, hud);
    return hud.init();
  },

  













  openBrowserConsole:
  function HS_openBrowserConsole(aTarget, aIframeWindow, aChromeWindow)
  {
    let hud = new BrowserConsole(aTarget, aIframeWindow, aChromeWindow);
    this.consoles.set(hud.hudId, hud);
    return hud.init();
  },

  





  getHudByWindow: function HS_getHudByWindow(aContentWindow)
  {
    for (let [hudId, hud] of this.consoles) {
      let target = hud.target;
      if (target && target.tab && target.window === aContentWindow) {
        return hud;
      }
    }
    return null;
  },

  





  getHudReferenceById: function HS_getHudReferenceById(aId)
  {
    return this.consoles.get(aId);
  },

  






  toggleWebConsole: function HS_toggleWebConsole()
  {
    let window = this.currentContext();
    let target = devtools.TargetFactory.forTab(window.gBrowser.selectedTab);
    let toolbox = gDevTools.getToolbox(target);

    return toolbox && toolbox.currentToolId == "webconsole" ?
        toolbox.destroy() :
        gDevTools.showToolbox(target, "webconsole");
  },

  






  getOpenWebConsole: function HS_getOpenWebConsole()
  {
    let tab = this.currentContext().gBrowser.selectedTab;
    if (!tab || !devtools.TargetFactory.isKnownTab(tab)) {
      return null;
    }
    let target = devtools.TargetFactory.forTab(tab);
    let toolbox = gDevTools.getToolbox(target);
    let panel = toolbox ? toolbox.getPanel("webconsole") : null;
    return panel ? panel.hud : null;
  },

  


  toggleBrowserConsole: function HS_toggleBrowserConsole()
  {
    if (this._browserConsoleID) {
      let hud = this.getHudReferenceById(this._browserConsoleID);
      return hud.destroy();
    }

    if (this._browserConsoleDefer) {
      return this._browserConsoleDefer.promise;
    }

    this._browserConsoleDefer = promise.defer();

    function connect()
    {
      let deferred = promise.defer();

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

      return devtools.TargetFactory.forRemoteTab(options);
    }

    function openWindow(aTarget)
    {
      target = aTarget;

      let deferred = promise.defer();

      let win = Services.ww.openWindow(null, devtools.Tools.webConsole.url, "_blank",
                                       BROWSER_CONSOLE_WINDOW_FEATURES, null);
      win.addEventListener("DOMContentLoaded", function onLoad() {
        win.removeEventListener("DOMContentLoaded", onLoad);

        
        let root = win.document.documentElement;
        root.setAttribute("title", root.getAttribute("browserConsoleTitle"));

        deferred.resolve(win);
      });

      return deferred.promise;
    }

    connect().then(getTarget).then(openWindow).then((aWindow) => {
      this.openBrowserConsole(target, aWindow, aWindow)
        .then((aBrowserConsole) => {
          this._browserConsoleID = aBrowserConsole.hudId;
          this._browserConsoleDefer.resolve(aBrowserConsole);
          this._browserConsoleDefer = null;
        })
    }, console.error);

    return this._browserConsoleDefer.promise;
  },

  






  getBrowserConsole: function HS_getBrowserConsole()
  {
    return this.getHudReferenceById(this._browserConsoleID);
  },
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

  this.ui = new WebConsoleFrame(this);
}

WebConsole.prototype = {
  iframeWindow: null,
  chromeWindow: null,
  browserWindow: null,
  hudId: null,
  target: null,
  ui: null,
  _browserConsole: false,
  _destroyer: null,

  






  get lastFinishedRequestCallback() HUDService.lastFinishedRequest.callback,

  



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
    let toolbox = gDevTools.getToolbox(this.target);
    if (!toolbox) {
      this.viewSource(aSourceURL, aSourceLine);
      return;
    }

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
    let toolbox = gDevTools.getToolbox(this.target);
    if (!toolbox) {
      this.viewSource(aSourceURL, aSourceLine);
      return;
    }

    let showSource = ({ DebuggerView }) => {
      if (DebuggerView.Sources.containsValue(aSourceURL)) {
        DebuggerView.setEditorLocation(aSourceURL, aSourceLine);
        return;
      }
      toolbox.selectTool("webconsole");
      this.viewSource(aSourceURL, aSourceLine);
    }

    
    
    
    let debuggerAlreadyOpen = toolbox.getPanel("jsdebugger");
    toolbox.selectTool("jsdebugger").then(({ panelWin: dbg }) => {
      if (debuggerAlreadyOpen) {
        showSource(dbg);
      } else {
        dbg.once(dbg.EVENTS.SOURCES_ADDED, () => showSource(dbg));
      }
    });
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
    let framesController = panel.panelWin.DebuggerController.StackFrames;
    let thread = framesController.activeThread;
    if (thread && thread.paused) {
      return {
        frames: thread.cachedFrames,
        selected: framesController.currentFrameDepth,
      };
    }
    return null;
  },

  






  destroy: function WC_destroy()
  {
    if (this._destroyer) {
      return this._destroyer.promise;
    }

    HUDService.consoles.delete(this.hudId);

    this._destroyer = promise.defer();

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
  this._telemetry = new Telemetry();
}

BrowserConsole.prototype = Heritage.extend(WebConsole.prototype,
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

    this.ui._filterPrefsPrefix = BROWSER_CONSOLE_FILTER_PREFS_PREFIX;

    let window = this.iframeWindow;

    
    
    let onClose = () => {
      window.removeEventListener("unload", onClose);
      this.destroy();
    };
    window.addEventListener("unload", onClose);

    
    window.document.getElementById("cmd_close").removeAttribute("disabled");

    this._telemetry.toolOpened("browserconsole");

    this._bc_init = this.$init();
    return this._bc_init;
  },

  $destroy: WebConsole.prototype.destroy,

  





  destroy: function BC_destroy()
  {
    if (this._bc_destroyer) {
      return this._bc_destroyer.promise;
    }

    this._telemetry.toolClosed("browserconsole");

    this._bc_destroyer = promise.defer();

    let chromeWindow = this.chromeWindow;
    this.$destroy().then(() =>
      this.target.client.close(() => {
        HUDService._browserConsoleID = null;
        chromeWindow.close();
        this._bc_destroyer.resolve(null);
      }));

    return this._bc_destroyer.promise;
  },
});

const HUDService = new HUD_SERVICE();

(() => {
  let methods = ["openWebConsole", "openBrowserConsole", "toggleWebConsole",
                 "toggleBrowserConsole", "getOpenWebConsole",
                 "getBrowserConsole", "getHudByWindow", "getHudReferenceById"];
  for (let method of methods) {
    exports[method] = HUDService[method].bind(HUDService);
  }

  exports.consoles = HUDService.consoles;
  exports.lastFinishedRequest = HUDService.lastFinishedRequest;
})();
