

































































var gPrintSettingsAreGlobal = false;
var gSavePrintSettings = false;
var gFocusedElement = null;

var PrintUtils = {
  bailOut: function () {
    let pref = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefBranch);
    let allow_for_testing = false;
    try {
      allow_for_testing = pref.getBoolPref("print.enable_e10s_testing");
    } catch(e) {
      
    }
    if (this.usingRemoteTabs && !allow_for_testing) {
      alert("e10s printing is not implemented yet. Bug 927188.");
      return true;
    }
    return false;
  },

  





  showPageSetup: function () {
    if (this.bailOut()) {
      return;
    }
    try {
      var printSettings = this.getPrintSettings();
      var PRINTPROMPTSVC = Components.classes["@mozilla.org/embedcomp/printingprompt-service;1"]
                                     .getService(Components.interfaces.nsIPrintingPromptService);
      PRINTPROMPTSVC.showPageSetup(window, printSettings, null);
      if (gSavePrintSettings) {
        
        var PSSVC = Components.classes["@mozilla.org/gfx/printsettings-service;1"]
                              .getService(Components.interfaces.nsIPrintSettingsService);
        PSSVC.savePrintSettingsToPrefs(printSettings, true, printSettings.kInitSaveNativeData);
      }
    } catch (e) {
      dump("showPageSetup "+e+"\n");
      return false;
    }
    return true;
  },

  
















  print: function (aWindow, aBrowser)
  {
    if (this.bailOut()) {
      return;
    }

    if (!aWindow) {
      
      
      if (this.usingRemoteTabs) {
        throw new Error("Windows running with remote tabs must explicitly pass " +
                        "a content window to PrintUtils.print.");
      }
      
      aWindow = window.content;
    }

    if (Components.utils.isCrossProcessWrapper(aWindow)) {
      if (!aBrowser) {
        throw new Error("PrintUtils.print expects a remote browser passed as " +
                        "an argument if the content window is a CPOW.");
      }
    } else {
      
      
      aBrowser = aWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                        .getInterface(Components.interfaces.nsIWebNavigation)
                        .QueryInterface(Components.interfaces.nsIDocShell)
                        .chromeEventHandler;
    }

    if (!aBrowser) {
      throw new Error("PrintUtils.print could not resolve content window " +
                      "to a browser.");
    }

    let mm = aBrowser.messageManager;

    mm.sendAsyncMessage("Printing:Print", null, {
      contentWindow: aWindow,
    });
  },

  





























  printPreview: function (aListenerObj)
  {
    if (this.bailOut()) {
      return;
    }
    
    
    
    if (!this.inPrintPreview) {
      this._listener = aListenerObj;
      this._sourceBrowser = aListenerObj.getSourceBrowser();
      this._originalTitle = this._sourceBrowser.contentTitle;
      this._originalURL = this._sourceBrowser.currentURI.spec;
    } else {
      
      
      
      this._sourceBrowser = this._listener.getPrintPreviewBrowser();
      this._sourceBrowser.collapsed = true;
    }

    this._webProgressPP = {};
    let ppParams        = {};
    let notifyOnOpen    = {};
    let printSettings   = this.getPrintSettings();
    
    
    
    
    
    let PPROMPTSVC = Components.classes["@mozilla.org/embedcomp/printingprompt-service;1"]
                               .getService(Components.interfaces.nsIPrintingPromptService);
    
    
    try {
      PPROMPTSVC.showProgress(window, null, printSettings, this._obsPP, false,
                              this._webProgressPP, ppParams, notifyOnOpen);
      if (ppParams.value) {
        ppParams.value.docTitle = this._originalTitle;
        ppParams.value.docURL   = this._originalURL;
      }

      
      
      if (!notifyOnOpen.value.valueOf() || this._webProgressPP.value == null) {
        this.enterPrintPreview();
      }
    } catch (e) {
      this.enterPrintPreview();
    }
  },

  








  getWebBrowserPrint: function (aWindow)
  {
    let Deprecated = Components.utils.import("resource://gre/modules/Deprecated.jsm", {}).Deprecated;
    let text = "getWebBrowserPrint is now deprecated, and fully unsupported for " +
               "multi-process browsers. Please use a frame script to get " +
               "access to nsIWebBrowserPrint from content.";
    let url = "https://developer.mozilla.org/en-US/docs/Printing_from_a_XUL_App";
    Deprecated.warning(text, url);

    if (this.usingRemoteTabs) {
      return {};
    }

    var contentWindow = aWindow || window.content;
    return contentWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                        .getInterface(Components.interfaces.nsIWebBrowserPrint);
  },

  






  getPrintPreview: function() {
    let Deprecated = Components.utils.import("resource://gre/modules/Deprecated.jsm", {}).Deprecated;
    let text = "getPrintPreview is now deprecated, and fully unsupported for " +
               "multi-process browsers. Please use a frame script to get " +
               "access to nsIWebBrowserPrint from content.";
    let url = "https://developer.mozilla.org/en-US/docs/Printing_from_a_XUL_App";
    Deprecated.warning(text, url);

    if (this.usingRemoteTabs) {
      return {};
    }

    return this._listener.getPrintPreviewBrowser().docShell.printPreview;
  },

  get inPrintPreview() {
    return document.getElementById("print-preview-toolbar") != null;
  },

  
  
  

  _listener: null,
  _closeHandlerPP: null,
  _webProgressPP: null,
  _sourceBrowser: null,
  _originalTitle: "",
  _originalURL: "",

  get usingRemoteTabs() {
    
    
    let usingRemoteTabs =
      window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
            .getInterface(Components.interfaces.nsIWebNavigation)
            .QueryInterface(Components.interfaces.nsILoadContext)
            .useRemoteTabs;
    delete this.usingRemoteTabs;
    return this.usingRemoteTabs = usingRemoteTabs;
  },

  receiveMessage(aMessage) {
    if (!this._webProgressPP.value) {
      
      
      return;
    }

    let listener = this._webProgressPP.value;
    let mm = aMessage.target.messageManager;
    let data = aMessage.data;

    switch (aMessage.name) {
      case "Printing:Preview:ProgressChange": {
        return listener.onProgressChange(null, null,
                                         data.curSelfProgress,
                                         data.maxSelfProgress,
                                         data.curTotalProgress,
                                         data.maxTotalProgress);
        break;
      }

      case "Printing:Preview:StateChange": {
        if (data.stateFlags & Components.interfaces.nsIWebProgressListener.STATE_STOP) {
          
          
          
          
          
          
          mm.removeMessageListener("Printing:Preview:StateChange", this);
          mm.removeMessageListener("Printing:Preview:ProgressChange", this);
        }

        return listener.onStateChange(null, null,
                                      data.stateFlags,
                                      data.status);
        break;
      }
    }
  },

  setPrinterDefaultsForSelectedPrinter: function (aPSSVC, aPrintSettings)
  {
    if (!aPrintSettings.printerName)
      aPrintSettings.printerName = aPSSVC.defaultPrinterName;

    
    aPSSVC.initPrintSettingsFromPrinter(aPrintSettings.printerName, aPrintSettings);
    
    aPSSVC.initPrintSettingsFromPrefs(aPrintSettings, true,  aPrintSettings.kInitSaveAll);
  },

  getPrintSettings: function ()
  {
    var pref = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefBranch);
    if (pref) {
      gPrintSettingsAreGlobal = pref.getBoolPref("print.use_global_printsettings", false);
      gSavePrintSettings = pref.getBoolPref("print.save_print_settings", false);
    }

    var printSettings;
    try {
      var PSSVC = Components.classes["@mozilla.org/gfx/printsettings-service;1"]
                            .getService(Components.interfaces.nsIPrintSettingsService);
      if (gPrintSettingsAreGlobal) {
        printSettings = PSSVC.globalPrintSettings;
        this.setPrinterDefaultsForSelectedPrinter(PSSVC, printSettings);
      } else {
        printSettings = PSSVC.newPrintSettings;
      }
    } catch (e) {
      dump("getPrintSettings: "+e+"\n");
    }
    return printSettings;
  },

  
  _obsPP:
  {
    observe: function(aSubject, aTopic, aData)
    {
      
      setTimeout(function () { PrintUtils.enterPrintPreview(); }, 0);
    },

    QueryInterface : function(iid)
    {
      if (iid.equals(Components.interfaces.nsIObserver) ||
          iid.equals(Components.interfaces.nsISupportsWeakReference) ||
          iid.equals(Components.interfaces.nsISupports))
        return this;
      throw Components.results.NS_NOINTERFACE;
    }
  },

  enterPrintPreview: function ()
  {
    
    
    
    
    
    let ppBrowser = this._listener.getPrintPreviewBrowser();
    let mm = ppBrowser.messageManager;
    mm.sendAsyncMessage("Printing:Preview:Enter", null, {
      contentWindow: this._sourceBrowser.contentWindowAsCPOW,
    });

    if (this._webProgressPP.value) {
      mm.addMessageListener("Printing:Preview:StateChange", this);
      mm.addMessageListener("Printing:Preview:ProgressChange", this);
    }

    let onEntered = (message) => {
      mm.removeMessageListener("Printing:PrintPreview:Entered", onEntered);

      if (message.data.failed) {
        
        
        this._listener.onEnter();
        this._listener.onExit();
        return;
      }

      
      
      gFocusedElement = document.commandDispatcher.focusedElement;

      let printPreviewTB = document.getElementById("print-preview-toolbar");
      if (printPreviewTB) {
        printPreviewTB.updateToolbar();
        ppBrowser.collapsed = false;
        ppBrowser.focus();
        return;
      }

      
      
      this._sourceBrowser.docShellIsActive = true;

      
      
      const XUL_NS =
        "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
      printPreviewTB = document.createElementNS(XUL_NS, "toolbar");
      printPreviewTB.setAttribute("printpreview", true);
      printPreviewTB.id = "print-preview-toolbar";

      let navToolbox = this._listener.getNavToolbox();
      navToolbox.parentNode.insertBefore(printPreviewTB, navToolbox);
      printPreviewTB.initialize(ppBrowser);

      
      if (document.documentElement.hasAttribute("onclose"))
        this._closeHandlerPP = document.documentElement.getAttribute("onclose");
      else
        this._closeHandlerPP = null;
      document.documentElement.setAttribute("onclose", "PrintUtils.exitPrintPreview(); return false;");

      
      window.addEventListener("keydown", this.onKeyDownPP, true);
      window.addEventListener("keypress", this.onKeyPressPP, true);

      ppBrowser.collapsed = false;
      ppBrowser.focus();
      
      this._listener.onEnter();
    };

    mm.addMessageListener("Printing:Preview:Entered", onEntered);
  },

  exitPrintPreview: function ()
  {
    let ppBrowser = this._listener.getPrintPreviewBrowser();
    let browserMM = ppBrowser.messageManager;
    browserMM.sendAsyncMessage("Printing:Preview:Exit");
    window.removeEventListener("keydown", this.onKeyDownPP, true);
    window.removeEventListener("keypress", this.onKeyPressPP, true);

    
    document.documentElement.setAttribute("onclose", this._closeHandlerPP);
    this._closeHandlerPP = null;

    
    let printPreviewTB = document.getElementById("print-preview-toolbar");
    this._listener.getNavToolbox().parentNode.removeChild(printPreviewTB);

    let fm = Components.classes["@mozilla.org/focus-manager;1"]
                       .getService(Components.interfaces.nsIFocusManager);
    if (gFocusedElement)
      fm.setFocus(gFocusedElement, fm.FLAG_NOSCROLL);
    else
      this._sourceBrowser.focus();
    gFocusedElement = null;

    this._listener.onExit();
  },

  onKeyDownPP: function (aEvent)
  {
    
    if (aEvent.keyCode == aEvent.DOM_VK_ESCAPE) {
      PrintUtils.exitPrintPreview();
    }
  },

  onKeyPressPP: function (aEvent)
  {
    var closeKey;
    try {
      closeKey = document.getElementById("key_close")
                         .getAttribute("key");
      closeKey = aEvent["DOM_VK_"+closeKey];
    } catch (e) {}
    var isModif = aEvent.ctrlKey || aEvent.metaKey;
    
    if (isModif &&
        (aEvent.charCode == closeKey || aEvent.charCode == closeKey + 32)) {
      PrintUtils.exitPrintPreview();
    }
    else if (isModif) {
      var printPreviewTB = document.getElementById("print-preview-toolbar");
      var printKey = document.getElementById("printKb").getAttribute("key").toUpperCase();
      var pressedKey = String.fromCharCode(aEvent.charCode).toUpperCase();
      if (printKey == pressedKey) {
        printPreviewTB.print();
      }
    }
    
    if (isModif) {
      aEvent.preventDefault();
      aEvent.stopPropagation();
    }
  }
}
