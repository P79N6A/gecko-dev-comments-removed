





const { utils: Cu, interfaces: Ci, classes: Cc } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

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





let ViewSourceChrome = {
  




  lastLineFound: null,

  





  contextMenuData: {},

  





  messages: [
    "ViewSource:SourceLoaded",
    "ViewSource:SourceUnloaded",
    "ViewSource:Close",
    "ViewSource:OpenURL",
    "ViewSource:GoToLine:Success",
    "ViewSource:GoToLine:Failed",
    "ViewSource:UpdateStatus",
    "ViewSource:ContextMenuOpening",
  ],

  



  init() {
    
    
    
    
    let wMM = window.messageManager;
    wMM.loadFrameScript("chrome://global/content/viewSource-content.js", true);
    this.messages.forEach((msgName) => {
      wMM.addMessageListener(msgName, this);
    });

    this.shouldWrap = Services.prefs.getBoolPref("view_source.wrap_long_lines");
    this.shouldHighlight =
      Services.prefs.getBoolPref("view_source.syntax_highlight");

    addEventListener("load", this);
    addEventListener("unload", this);
    addEventListener("AppCommand", this, true);
    addEventListener("MozSwipeGesture", this, true);
  },

  



  uninit() {
    let wMM = window.messageManager;
    this.messages.forEach((msgName) => {
      wMM.removeMessageListener(msgName, this);
    });

    
    
    removeEventListener("unload", this);
    removeEventListener("AppCommand", this, true);
    removeEventListener("MozSwipeGesture", this, true);
    gContextMenu.removeEventListener("popupshowing", this);
    gContextMenu.removeEventListener("popuphidden", this);
  },

  



  receiveMessage(message) {
    let data = message.data;

    switch(message.name) {
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
      case "ViewSource:GoToLine:Failed":
        this.onGoToLineFailed();
        break;
      case "ViewSource:GoToLine:Success":
        this.onGoToLineSuccess(data.lineNumber);
        break;
      case "ViewSource:UpdateStatus":
        this.updateStatus(data.label);
        break;
      case "ViewSource:ContextMenuOpening":
        this.onContextMenuOpening(data.isLink, data.isEmail, data.href);
        if (gBrowser.isRemoteBrowser) {
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
    }
  },

  



  get historyEnabled() {
    return !gBrowser.hasAttribute("disablehistory");
  },

  


  get mm() {
    return gBrowser.messageManager;
  },

  


  get webNav() {
    return gBrowser.webNavigation;
  },

  


  goForward() {
    gBrowser.goForward();
  },

  


  goBack() {
    gBrowser.goBack();
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

    if (!this.historyEnabled) {
      
      let viewSourceNavigation = document.getElementById("viewSourceNavigation");
      if (viewSourceNavigation) {
        viewSourceNavigation.setAttribute("disabled", "true");
        viewSourceNavigation.setAttribute("hidden", "true");
      }
    }

    
    gBrowser.droppedLinkHandler = function (event, url, name) {
      ViewSourceChrome.loadURL(url);
      event.preventDefault();
    };

    
    
    if (!window.arguments[0]) {
      return;
    }

    if (typeof window.arguments[0] == "string") {
      
      return ViewSourceChrome._loadViewSourceDeprecated();
    }

    
    
    let args = window.arguments[0];

    if (!args.URL) {
      throw new Error("Must supply a URL when opening view source.");
    }

    if (args.browser) {
      
      
      this.updateBrowserRemoteness(args.browser.isRemoteBrowser);
    } else {
      if (args.outerWindowID) {
        throw new Error("Must supply the browser if passing the outerWindowID");
      }
    }

    this.mm.sendAsyncMessage("ViewSource:LoadSource", {
      URL: args.URL,
      outerWindowID: args.outerWindowID,
      lineNumber: args.lineNumber,
    });
  },

  



  _loadViewSourceDeprecated() {
    Deprecated.warning("The arguments you're passing to viewSource.xul " +
                       "are using an out-of-date API.",
                       "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/ViewSource");
    
    
    
    
    
    

    if (window.arguments[3] == "selection" ||
        window.arguments[3] == "mathml") {
      
      return;
    }

    if (window.arguments[2]) {
      let pageDescriptor = window.arguments[2];
      if (Cu.isCrossProcessWrapper(pageDescriptor)) {
        throw new Error("Cannot pass a CPOW as the page descriptor to viewSource.xul.");
      }
    }

    if (gBrowser.isRemoteBrowser) {
      throw new Error("Deprecated view source API should not use a remote browser.");
    }

    let forcedCharSet;
    if (window.arguments[4] && window.arguments[1].startsWith("charset=")) {
      forcedCharSet = window.arguments[1].split("=")[1];
    }

    gBrowser.messageManager.sendAsyncMessage("ViewSource:LoadSourceDeprecated", {
      URL: window.arguments[0],
      lineNumber: window.arguments[3],
      forcedCharSet,
    }, {
      pageDescriptor: window.arguments[2],
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

    gBrowser.focus();
  },

  



  onSourceUnloaded() {
    
    
    document.getElementById("cmd_goToLine").setAttribute("disabled", "true");
  },

  






  onSetCharacterSet(event) {
    if (event.target.hasAttribute("charset")) {
      let charset = event.target.getAttribute("charset");

      
      
      this.mm.sendAsyncMessage("ViewSource:SetCharacterSet", {
        charset: charset,
        doPageLoad: this.historyEnabled,
      });

      if (this.historyEnabled) {
        gBrowser.reloadWithFlags(Ci.nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
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
    clipboard.copyString(this.contextMenuData.href, document);
  },

  




  onContextMenuHidden(event) {
    this.contextMenuData = {
      isOpen: false,
    };
  },

  













  openContextMenu(screenX, screenY) {
    gContextMenu.openPopupAtScreen(screenX, screenY, true);
  },

  






  loadURL(URL) {
    this.mm.sendAsyncMessage("ViewSource:LoadSource", { URL });
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

  




  promptAndGoToLine() {
    let input = { value: this.lastLineFound };

    let ok = Services.prompt.prompt(
        window,
        gViewSourceBundle.getString("goToLineTitle"),
        gViewSourceBundle.getString("goToLineText"),
        input,
        null,
        {value:0});

    if (!ok)
      return;

    let line = parseInt(input.value, 10);

    if (!(line > 0)) {
      Services.prompt.alert(window,
                            gViewSourceBundle.getString("invalidInputTitle"),
                            gViewSourceBundle.getString("invalidInputText"));
      this.promptAndGoToLine();
    } else {
      this.goToLine(line);
    }
  },

  





  goToLine(lineNumber) {
    this.mm.sendAsyncMessage("ViewSource:GoToLine", { lineNumber });
  },

  






  onGoToLineSuccess(lineNumber) {
    
    
    this.lastLineFound = lineNumber;
    document.getElementById("statusbar-line-col").label =
      gViewSourceBundle.getFormattedString("statusBarLineCol", [lineNumber, 1]);
  },

  




  onGoToLineFailed() {
    Services.prompt.alert(window,
                          gViewSourceBundle.getString("outOfRangeTitle"),
                          gViewSourceBundle.getString("outOfRangeText"));
    this.promptAndGoToLine();
  },

  


  reload() {
    gBrowser.reloadWithFlags(Ci.nsIWebNavigation.LOAD_FLAGS_BYPASS_PROXY |
                             Ci.nsIWebNavigation.LOAD_FLAGS_BYPASS_CACHE);
  },

  


  close() {
    window.close();
  },

  




  toggleWrapping() {
    this.shouldWrap = !this.shouldWrap;
    Services.prefs.setBoolPref("view_source.wrap_long_lines",
                               this.shouldWrap);
    this.mm.sendAsyncMessage("ViewSource:ToggleWrapping");
  },

  




  toggleSyntaxHighlighting() {
    this.shouldHighlight = !this.shouldHighlight;
    
    
    
    Services.prefs.setBoolPref("view_source.syntax_highlight",
                               this.shouldHighlight);
    this.mm.sendAsyncMessage("ViewSource:ToggleSyntaxHighlighting");
  },

  









  updateBrowserRemoteness(shouldBeRemote) {
    if (gBrowser.isRemoteBrowser == shouldBeRemote) {
      return;
    }

    let parentNode = gBrowser.parentNode;
    let nextSibling = gBrowser.nextSibling;

    gBrowser.remove();
    if (shouldBeRemote) {
      gBrowser.setAttribute("remote", "true");
    } else {
      gBrowser.removeAttribute("remote");
    }
    
    
    parentNode.insertBefore(gBrowser, nextSibling);

    if (shouldBeRemote) {
      
      
      
      
      
      
      
      
      
      gBrowser.webProgress;
    }
  },
};

ViewSourceChrome.init();




let PrintPreviewListener = {
  getPrintPreviewBrowser() {
    let browser = document.getElementById("ppBrowser");
    if (!browser) {
      browser = document.createElement("browser");
      browser.setAttribute("id", "ppBrowser");
      browser.setAttribute("flex", "1");
      browser.setAttribute("type", "content");

      let findBar = document.getElementById("FindToolbar");
      document.getElementById("appcontent")
              .insertBefore(browser, findBar);
    }

    return browser;
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
    document.getElementById("ppBrowser").collapsed = true;
    gBrowser.collapsed = false;
    document.getElementById("viewSource-toolbox").hidden = false;
  },
};


function getBrowser() {
  return gBrowser;
}

this.__defineGetter__("gPageLoader", function () {
  var webnav = ViewSourceChrome.webNav;
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
                     "ViewSourceChrome.lastLineFound instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/ViewSource");
  return ViewSourceChrome.lastLineFound;
});

function onLoadViewSource() {
  Deprecated.warning("onLoadViewSource() is deprecated - please use " +
                     "ViewSourceChrome.onXULLoaded() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/ViewSource");
  ViewSourceChrome.onXULLoaded();
}

function isHistoryEnabled() {
  Deprecated.warning("isHistoryEnabled() is deprecated - please use " +
                     "ViewSourceChrome.historyEnabled instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/ViewSource");
  return ViewSourceChrome.historyEnabled;
}

function ViewSourceClose() {
  Deprecated.warning("ViewSourceClose() is deprecated - please use " +
                     "ViewSourceChrome.close() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/ViewSource");
  ViewSourceChrome.close();
}

function ViewSourceReload() {
  Deprecated.warning("ViewSourceReload() is deprecated - please use " +
                     "ViewSourceChrome.reload() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/ViewSource");
  ViewSourceChrome.reload();
}

function getWebNavigation()
{
  Deprecated.warning("getWebNavigation() is deprecated - please use " +
                     "ViewSourceChrome.webNav instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/ViewSource");
  
  
  try {
    return ViewSourceChrome.webNav;
  } catch (e) {
    return null;
  }
}

function viewSource(url) {
  Deprecated.warning("viewSource() is deprecated - please use " +
                     "ViewSourceChrome.loadURL() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/ViewSource");
  ViewSourceChrome.loadURL(url);
}

function ViewSourceGoToLine()
{
  Deprecated.warning("ViewSourceGoToLine() is deprecated - please use " +
                     "ViewSourceChrome.promptAndGoToLine() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/ViewSource");
  ViewSourceChrome.promptAndGoToLine();
}

function goToLine(line)
{
  Deprecated.warning("goToLine() is deprecated - please use " +
                     "ViewSourceChrome.goToLine() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/ViewSource");
  ViewSourceChrome.goToLine(line);
}

function BrowserForward(aEvent) {
  Deprecated.warning("BrowserForward() is deprecated - please use " +
                     "ViewSourceChrome.goForward() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/ViewSource");
  ViewSourceChrome.goForward();
}

function BrowserBack(aEvent) {
  Deprecated.warning("BrowserBack() is deprecated - please use " +
                     "ViewSourceChrome.goBack() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/ViewSource");
  ViewSourceChrome.goBack();
}

function UpdateBackForwardCommands() {
  Deprecated.warning("UpdateBackForwardCommands() is deprecated - please use " +
                     "ViewSourceChrome.updateCommands() instead.",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/ViewSource");
  ViewSourceChrome.updateCommands();
}
