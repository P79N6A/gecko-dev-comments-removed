






var gPrintSettingsAreGlobal = false;
var gSavePrintSettings = false;
var gFocusedElement = null;

var PrintUtils = {
  bailOut: function () {
    let remote = window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
            .getInterface(Components.interfaces.nsIWebNavigation)
            .QueryInterface(Components.interfaces.nsILoadContext)
            .useRemoteTabs;
    if (remote) {
      alert("e10s printing is not implemented yet. Bug 927188.");
      return true;
    }
    return false;
  },

  showPageSetup: function ()
  {
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

  print: function (aWindow)
  {
    if (this.bailOut()) {
      return;
    }
    var webBrowserPrint = this.getWebBrowserPrint(aWindow);
    var printSettings = this.getPrintSettings();
    try {
      webBrowserPrint.print(printSettings, null);
      if (gPrintSettingsAreGlobal && gSavePrintSettings) {
        var PSSVC = Components.classes["@mozilla.org/gfx/printsettings-service;1"]
                              .getService(Components.interfaces.nsIPrintSettingsService);
        PSSVC.savePrintSettingsToPrefs(printSettings, true,
                                       printSettings.kInitSaveAll);
        PSSVC.savePrintSettingsToPrefs(printSettings, false,
                                       printSettings.kInitSavePrinterName);
      }
    } catch (e) {
      
      
      
      
    }
  },

  
  
  
  
  
  printPreview: function (aCallback)
  {
    if (this.bailOut()) {
      return;
    }
    
    
    
    if (!document.getElementById("print-preview-toolbar")) {
      this._callback = aCallback;
      this._sourceBrowser = aCallback.getSourceBrowser();
      this._originalTitle = this._sourceBrowser.contentDocument.title;
      this._originalURL = this._sourceBrowser.currentURI.spec;
    } else {
      
      
      
      this._sourceBrowser = this._callback.getPrintPreviewBrowser();
      this._sourceBrowser.collapsed = true;
    }

    this._webProgressPP = {};
    var ppParams        = {};
    var notifyOnOpen    = {};
    var webBrowserPrint = this.getWebBrowserPrint();
    var printSettings   = this.getPrintSettings();
    
    
    
    
    
    var PPROMPTSVC = Components.classes["@mozilla.org/embedcomp/printingprompt-service;1"]
                               .getService(Components.interfaces.nsIPrintingPromptService);
    
    
    try {
      PPROMPTSVC.showProgress(window, webBrowserPrint, printSettings, this._obsPP, false,
                              this._webProgressPP, ppParams, notifyOnOpen);
      if (ppParams.value) {
        ppParams.value.docTitle = this._originalTitle;
        ppParams.value.docURL   = this._originalURL;
      }

      
      
      if (!notifyOnOpen.value.valueOf() || this._webProgressPP.value == null)
        this.enterPrintPreview();
    } catch (e) {
      this.enterPrintPreview();
    }
  },

  getWebBrowserPrint: function (aWindow)
  {
    var contentWindow = aWindow || window.content;
    return contentWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                        .getInterface(Components.interfaces.nsIWebBrowserPrint);
  },

  getPrintPreview: function() {
    return this._callback.getPrintPreviewBrowser().docShell.printPreview;
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

  _closeHandlerPP: null,
  _webProgressPP: null,
  _callback: null,
  _sourceBrowser: null,
  _originalTitle: "",
  _originalURL: "",

  
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
    gFocusedElement = document.commandDispatcher.focusedElement;

    var webBrowserPrint;
    var printSettings  = this.getPrintSettings();
    var originalWindow = this._sourceBrowser.contentWindow;

    try {
      webBrowserPrint = this.getPrintPreview();
      webBrowserPrint.printPreview(printSettings, originalWindow,
                                   this._webProgressPP.value);
    } catch (e) {
      
      
      
      

      
      this._callback.onEnter();
      this._callback.onExit();
      return;
    }

    var printPreviewTB = document.getElementById("print-preview-toolbar");
    if (printPreviewTB) {
      printPreviewTB.updateToolbar();
      var browser = this._callback.getPrintPreviewBrowser();
      browser.collapsed = false;
      browser.contentWindow.focus();
      return;
    }

    
    
    var docShell = originalWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                                 .getInterface(Components.interfaces.nsIWebNavigation)
                                 .QueryInterface(Components.interfaces.nsIDocShell);
    docShell.isActive = true;

    
    
    var XUL_NS =
      "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    printPreviewTB = document.createElementNS(XUL_NS, "toolbar");
    printPreviewTB.setAttribute("printpreview", true);
    printPreviewTB.id = "print-preview-toolbar";
    printPreviewTB.className = "toolbar-primary";

    var navToolbox = this._callback.getNavToolbox();
    navToolbox.parentNode.insertBefore(printPreviewTB, navToolbox);

    
    if (document.documentElement.hasAttribute("onclose"))
      this._closeHandlerPP = document.documentElement.getAttribute("onclose");
    else
      this._closeHandlerPP = null;
    document.documentElement.setAttribute("onclose", "PrintUtils.exitPrintPreview(); return false;");

    
    window.addEventListener("keydown", this.onKeyDownPP, true);
    window.addEventListener("keypress", this.onKeyPressPP, true);

    var browser = this._callback.getPrintPreviewBrowser();
    browser.collapsed = false;
    browser.contentWindow.focus();

    
    this._callback.onEnter();
  },

  exitPrintPreview: function ()
  {
    window.removeEventListener("keydown", this.onKeyDownPP, true);
    window.removeEventListener("keypress", this.onKeyPressPP, true);

    
    document.documentElement.setAttribute("onclose", this._closeHandlerPP);
    this._closeHandlerPP = null;

    var webBrowserPrint = this.getPrintPreview();
    webBrowserPrint.exitPrintPreview();

    
    var printPreviewTB = document.getElementById("print-preview-toolbar");
    this._callback.getNavToolbox().parentNode.removeChild(printPreviewTB);

    var fm = Components.classes["@mozilla.org/focus-manager;1"]
                       .getService(Components.interfaces.nsIFocusManager);
    if (gFocusedElement)
      fm.setFocus(gFocusedElement, fm.FLAG_NOSCROLL);
    else
      window.content.focus();
    gFocusedElement = null;

    this._callback.onExit();
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
	  PrintUtils.print();
      }
    }
    
    if (isModif) {
      aEvent.preventDefault();
      aEvent.stopPropagation();
    }
  }
}
