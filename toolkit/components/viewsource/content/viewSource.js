





const { utils: Cu, interfaces: Ci, classes: Cc } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/ViewSourceBrowser.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CharsetMenu",
  "resource://gre/modules/CharsetMenu.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Deprecated",
  "resource://gre/modules/Deprecated.jsm");

[
  ["gBrowser",          "content"],
  ["gViewSourceBundle", "viewSourceBundle"],
  ["gContextMenu",      "viewSourceContextMenu"]
].forEach(function ([name, id]) {
  window.__defineGetter__(name, function () {
    var element = document.getElementById(id);
    if (!element)
      return null;
    delete window[name];
    return window[name] = element;
  });
});








function ViewSourceChrome() {
  ViewSourceBrowser.call(this);
}

ViewSourceChrome.prototype = {
  __proto__: ViewSourceBrowser.prototype,

  


  get browser() {
    return gBrowser;
  },

  





  contextMenuData: {},

  





  messages: ViewSourceBrowser.prototype.messages.concat([
    "ViewSource:SourceLoaded",
    "ViewSource:SourceUnloaded",
    "ViewSource:Close",
    "ViewSource:OpenURL",
    "ViewSource:UpdateStatus",
    "ViewSource:ContextMenuOpening",
  ]),

  




  init() {
    this.mm.loadFrameScript("chrome://global/content/viewSource-content.js", true);

    this.shouldWrap = Services.prefs.getBoolPref("view_source.wrap_long_lines");
    this.shouldHighlight =
      Services.prefs.getBoolPref("view_source.syntax_highlight");

    addEventListener("load", this);
    addEventListener("unload", this);
    addEventListener("AppCommand", this, true);
    addEventListener("MozSwipeGesture", this, true);

    ViewSourceBrowser.prototype.init.call(this);
  },

  



  uninit() {
    ViewSourceBrowser.prototype.uninit.call(this);

    
    
    removeEventListener("unload", this);
    removeEventListener("AppCommand", this, true);
    removeEventListener("MozSwipeGesture", this, true);
    gContextMenu.removeEventListener("popupshowing", this);
    gContextMenu.removeEventListener("popuphidden", this);
    Services.els.removeSystemEventListener(this.browser, "dragover", this,
                                           true);
    Services.els.removeSystemEventListener(this.browser, "drop", this, true);
  },

  



  receiveMessage(message) {
    let data = message.data;

    switch(message.name) {
      
      case "ViewSource:PromptAndGoToLine":
        this.promptAndGoToLine();
        break;
      case "ViewSource:GoToLine:Success":
        this.onGoToLineSuccess(data.lineNumber);
        break;
      case "ViewSource:GoToLine:Failed":
        this.onGoToLineFailed();
        break;
      case "ViewSource:StoreWrapping":
        this.storeWrapping(data.state);
        break;
      case "ViewSource:StoreSyntaxHighlighting":
        this.storeSyntaxHighlighting(data.state);
        break;
      
      case "ViewSource:SourceLoaded":
        this.onSourceLoaded();
        break;
      case "ViewSource:SourceUnloaded":
        this.onSourceUnloaded();
        break;
      case "ViewSource:Close":
        this.close();
        break;
      case "ViewSource:OpenURL":
        this.openURL(data.URL);
        break;
      case "ViewSource:UpdateStatus":
        this.updateStatus(data.label);
        break;
      case "ViewSource:ContextMenuOpening":
        this.onContextMenuOpening(data.isLink, data.isEmail, data.href);
        if (this.browser.isRemoteBrowser) {
          this.openContextMenu(data.screenX, data.screenY);
        }
        break;
    }
  },

  



  handleEvent(event) {
    switch(event.type) {
      case "unload":
        this.uninit();
        break;
      case "load":
        this.onXULLoaded();
        break;
      case "AppCommand":
        this.onAppCommand(event);
        break;
      case "MozSwipeGesture":
        this.onSwipeGesture(event);
        break;
      case "popupshowing":
        this.onContextMenuShowing(event);
        break;
      case "popuphidden":
        this.onContextMenuHidden(event);
        break;
      case "dragover":
        this.onDragOver(event);
        break;
      case "drop":
        this.onDrop(event);
        break;
    }
  },

  



  get historyEnabled() {
    return !this.browser.hasAttribute("disablehistory");
  },

  









  get mm() {
    return window.messageManager;
  },

  


  goForward() {
    this.browser.goForward();
  },

  


  goBack() {
    this.browser.goBack();
  },

  






























  onXULLoaded() {
    
    removeEventListener("load", this);

    let wrapMenuItem = document.getElementById("menu_wrapLongLines");
    if (this.shouldWrap) {
      wrapMenuItem.setAttribute("checked", "true");
    }

    let highlightMenuItem = document.getElementById("menu_highlightSyntax");
    if (this.shouldHighlight) {
      highlightMenuItem.setAttribute("checked", "true");
    }

    gContextMenu.addEventListener("popupshowing", this);
    gContextMenu.addEventListener("popuphidden", this);

    Services.els.addSystemEventListener(this.browser, "dragover", this, true);
    Services.els.addSystemEventListener(this.browser, "drop", this, true);

    if (!this.historyEnabled) {
      
      let viewSourceNavigation = document.getElementById("viewSourceNavigation");
      if (viewSourceNavigation) {
        viewSourceNavigation.setAttribute("disabled", "true");
        viewSourceNavigation.setAttribute("hidden", "true");
      }
    }

    
    
    if (!window.arguments[0]) {
      return;
    }

    if (typeof window.arguments[0] == "string") {
      
      return this._loadViewSourceDeprecated(window.arguments);
    }

    
    
    let args = window.arguments[0];
    this.loadViewSource(args);
  },

  



  _loadViewSourceDeprecated(aArguments) {
    Deprecated.warning("The arguments you're passing to viewSource.xul " +
                       "are using an out-of-date API.",
                       "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/View_Source_for_XUL_Applications");
    
    
    
    
    
    

    if (aArguments[3] == "selection" ||
        aArguments[3] == "mathml") {
      
      return;
    }

    if (aArguments[2]) {
      let pageDescriptor = aArguments[2];
      if (Cu.isCrossProcessWrapper(pageDescriptor)) {
        throw new Error("Cannot pass a CPOW as the page descriptor to viewSource.xul.");
      }
    }

    if (this.browser.isRemoteBrowser) {
      throw new Error("Deprecated view source API should not use a remote browser.");
    }

    let forcedCharSet;
    if (aArguments[4] && aArguments[1].startsWith("charset=")) {
      forcedCharSet = aArguments[1].split("=")[1];
    }

    this.sendAsyncMessage("ViewSource:LoadSourceDeprecated", {
      URL: aArguments[0],
      lineNumber: aArguments[3],
      forcedCharSet,
    }, {
      pageDescriptor: aArguments[2],
    });
  },

  





  onAppCommand(event) {
    event.stopPropagation();
    switch (event.command) {
      case "Back":
        this.goBack();
        break;
      case "Forward":
        this.goForward();
        break;
    }
  },

  





  onSwipeGesture(event) {
    event.stopPropagation();
    switch (event.direction) {
      case SimpleGestureEvent.DIRECTION_LEFT:
        this.goBack();
        break;
      case SimpleGestureEvent.DIRECTION_RIGHT:
        this.goForward();
        break;
      case SimpleGestureEvent.DIRECTION_UP:
        goDoCommand("cmd_scrollTop");
        break;
      case SimpleGestureEvent.DIRECTION_DOWN:
        goDoCommand("cmd_scrollBottom");
        break;
    }
  },

  



  onSourceLoaded() {
    document.getElementById("cmd_goToLine").removeAttribute("disabled");

    if (this.historyEnabled) {
      this.updateCommands();
    }

    this.browser.focus();
  },

  



  onSourceUnloaded() {
    
    
    document.getElementById("cmd_goToLine").setAttribute("disabled", "true");
  },

  






  onSetCharacterSet(event) {
    if (event.target.hasAttribute("charset")) {
      let charset = event.target.getAttribute("charset");

      
      
      this.sendAsyncMessage("ViewSource:SetCharacterSet", {
        charset: charset,
        doPageLoad: this.historyEnabled,
      });

      if (!this.historyEnabled) {
        this.browser
            .reloadWithFlags(Ci.nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
      }
    }
  },

  





  onContextMenuOpening(isLink, isEmail, href) {
    this.contextMenuData = { isLink, isEmail, href, isOpen: true };
  },

  








  onContextMenuShowing(event) {
    let copyLinkMenuItem = document.getElementById("context-copyLink");
    copyLinkMenuItem.hidden = !this.contextMenuData.isLink;

    let copyEmailMenuItem = document.getElementById("context-copyEmail");
    copyEmailMenuItem.hidden = !this.contextMenuData.isEmail;
  },

  




  onContextMenuCopyLinkOrEmail() {
    
    
    if (!this.contextMenuData.isOpen) {
      return;
    }

    let clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"]
                      .getService(Ci.nsIClipboardHelper);
    clipboard.copyString(this.contextMenuData.href);
  },

  




  onContextMenuHidden(event) {
    this.contextMenuData = {
      isOpen: false,
    };
  },

  


  onDragOver(event) {
    
    
    
    let types = event.dataTransfer.types;
    if (types.contains("text/x-moz-text-internal") && !types.contains("text/plain")) {
        event.dataTransfer.dropEffect = "none";
        event.stopPropagation();
        event.preventDefault();
    }

    let linkHandler = Cc["@mozilla.org/content/dropped-link-handler;1"]
                        .getService(Ci.nsIDroppedLinkHandler);

    if (linkHandler.canDropLink(event, false)) {
      event.preventDefault();
    }
  },

  


  onDrop(event) {
    if (event.defaultPrevented)
      return;

    let name = { };
    let linkHandler = Cc["@mozilla.org/content/dropped-link-handler;1"]
                        .getService(Ci.nsIDroppedLinkHandler);
    let uri;
    try {
      
      uri = linkHandler.dropLink(event, name, true);
    } catch (e) {
      return;
    }

    if (uri) {
      this.loadURL(uri);
    }
  },

  













  openContextMenu(screenX, screenY) {
    gContextMenu.openPopupAtScreen(screenX, screenY, true);
  },

  






  loadURL(URL) {
    this.sendAsyncMessage("ViewSource:LoadSource", { URL });
  },

  


  updateCommands() {
    let backBroadcaster = document.getElementById("Browser:Back");
    let forwardBroadcaster = document.getElementById("Browser:Forward");

    if (this.webNav.canGoBack) {
      backBroadcaster.removeAttribute("disabled");
    } else {
      backBroadcaster.setAttribute("disabled", "true");
    }
    if (this.webNav.canGoForward) {
      forwardBroadcaster.removeAttribute("disabled");
    } else {
      forwardBroadcaster.setAttribute("disabled", "true");
    }
  },

  





  updateStatus(label) {
    let statusBarField = document.getElementById("statusbar-line-col");
    if (statusBarField) {
      statusBarField.label = label;
    }
  },

  






  onGoToLineSuccess(lineNumber) {
    ViewSourceBrowser.prototype.onGoToLineSuccess.call(this, lineNumber);
    document.getElementById("statusbar-line-col").label =
      gViewSourceBundle.getFormattedString("statusBarLineCol", [lineNumber, 1]);
  },

  


  reload() {
    this.browser.reloadWithFlags(Ci.nsIWebNavigation.LOAD_FLAGS_BYPASS_PROXY |
                                 Ci.nsIWebNavigation.LOAD_FLAGS_BYPASS_CACHE);
  },

  


  close() {
    window.close();
  },

  


  toggleWrapping() {
    this.shouldWrap = !this.shouldWrap;
    this.sendAsyncMessage("ViewSource:ToggleWrapping");
  },

  


  toggleSyntaxHighlighting() {
    this.shouldHighlight = !this.shouldHighlight;
    this.sendAsyncMessage("ViewSource:ToggleSyntaxHighlighting");
  },

  









  updateBrowserRemoteness(shouldBeRemote) {
    if (this.browser.isRemoteBrowser == shouldBeRemote) {
      return;
    }

    let parentNode = this.browser.parentNode;
    let nextSibling = this.browser.nextSibling;

    this.browser.remove();
    if (shouldBeRemote) {
      this.browser.setAttribute("remote", "true");
    } else {
      this.browser.removeAttribute("remote");
    }
    
    
    parentNode.insertBefore(this.browser, nextSibling);

    if (shouldBeRemote) {
      
      
      
      
      
      
      
      
      
      this.browser.webProgress;
    }
  },
};

let viewSourceChrome = new ViewSourceChrome();




let PrintPreviewListener = {
  _ppBrowser: null,

  getPrintPreviewBrowser() {
    if (!this._ppBrowser) {
      this._ppBrowser = document.createElement("browser");
      this._ppBrowser.setAttribute("flex", "1");
      this._ppBrowser.setAttribute("type", "content");
    }

    if (gBrowser.isRemoteBrowser) {
      this._ppBrowser.setAttribute("remote", "true");
    } else {
      this._ppBrowser.removeAttribute("remote");
    }

    let findBar = document.getElementById("FindToolbar");
    document.getElementById("appcontent")
            .insertBefore(this._ppBrowser, findBar);

    return this._ppBrowser;
  },

  getSourceBrowser() {
    return gBrowser;
  },

  getNavToolbox() {
    return document.getElementById("appcontent");
  },

  onEnter() {
    let toolbox = document.getElementById("viewSource-toolbox");
    toolbox.hidden = true;
    gBrowser.collapsed = true;
  },

  onExit() {
    this._ppBrowser.remove();
    gBrowser.collapsed = false;
    document.getElementById("viewSource-toolbox").hidden = false;
  },
};


function getBrowser() {
  return gBrowser;
}

this.__defineGetter__("gPageLoader", function () {
  var webnav = viewSourceChrome.webNav;
  if (!webnav)
    return null;
  delete this.gPageLoader;
  this.gPageLoader = (webnav instanceof Ci.nsIWebPageDescriptor) ? webnav
                                                                 : null;
  return this.gPageLoader;
});


function ViewSourceSavePage()
{
  internalSave(gBrowser.currentURI.spec.replace(/^view-source:/i, ""),
               null, null, null, null, null, "SaveLinkTitle",
               null, null, gBrowser.contentDocumentAsCPOW, null,
               gPageLoader);
}




this.__defineGetter__("gLastLineFound", function () {
  Deprecated.warning("gLastLineFound is deprecated - please use " +
                     "viewSourceChrome.lastLineFound instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/View_Source_for_XUL_Applications");
  return viewSourceChrome.lastLineFound;
});

function onLoadViewSource() {
  Deprecated.warning("onLoadViewSource() is deprecated - please use " +
                     "viewSourceChrome.onXULLoaded() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/View_Source_for_XUL_Applications");
  viewSourceChrome.onXULLoaded();
}

function isHistoryEnabled() {
  Deprecated.warning("isHistoryEnabled() is deprecated - please use " +
                     "viewSourceChrome.historyEnabled instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/View_Source_for_XUL_Applications");
  return viewSourceChrome.historyEnabled;
}

function ViewSourceClose() {
  Deprecated.warning("ViewSourceClose() is deprecated - please use " +
                     "viewSourceChrome.close() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/View_Source_for_XUL_Applications");
  viewSourceChrome.close();
}

function ViewSourceReload() {
  Deprecated.warning("ViewSourceReload() is deprecated - please use " +
                     "viewSourceChrome.reload() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/View_Source_for_XUL_Applications");
  viewSourceChrome.reload();
}

function getWebNavigation()
{
  Deprecated.warning("getWebNavigation() is deprecated - please use " +
                     "viewSourceChrome.webNav instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/View_Source_for_XUL_Applications");
  
  
  try {
    return viewSourceChrome.webNav;
  } catch (e) {
    return null;
  }
}

function viewSource(url) {
  Deprecated.warning("viewSource() is deprecated - please use " +
                     "viewSourceChrome.loadURL() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/View_Source_for_XUL_Applications");
  viewSourceChrome.loadURL(url);
}

function ViewSourceGoToLine()
{
  Deprecated.warning("ViewSourceGoToLine() is deprecated - please use " +
                     "viewSourceChrome.promptAndGoToLine() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/View_Source_for_XUL_Applications");
  viewSourceChrome.promptAndGoToLine();
}

function goToLine(line)
{
  Deprecated.warning("goToLine() is deprecated - please use " +
                     "viewSourceChrome.goToLine() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/View_Source_for_XUL_Applications");
  viewSourceChrome.goToLine(line);
}

function BrowserForward(aEvent) {
  Deprecated.warning("BrowserForward() is deprecated - please use " +
                     "viewSourceChrome.goForward() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/View_Source_for_XUL_Applications");
  viewSourceChrome.goForward();
}

function BrowserBack(aEvent) {
  Deprecated.warning("BrowserBack() is deprecated - please use " +
                     "viewSourceChrome.goBack() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/View_Source_for_XUL_Applications");
  viewSourceChrome.goBack();
}

function UpdateBackForwardCommands() {
  Deprecated.warning("UpdateBackForwardCommands() is deprecated - please use " +
                     "viewSourceChrome.updateCommands() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/View_Source_for_XUL_Applications");
  viewSourceChrome.updateCommands();
}
