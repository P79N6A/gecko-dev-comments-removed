







































var gPrintSettingsAreGlobal = false;
var gSavePrintSettings = false;

var PrintUtils = {

  showPageSetup: function ()
  {
    try {
      var printSettings = this.getPrintSettings();
      var printingPromptService = Components.classes["@mozilla.org/embedcomp/printingprompt-service;1"]
                                            .getService(Components.interfaces.nsIPrintingPromptService);
      printingPromptService.showPageSetup(window, printSettings, null);
      if (gSavePrintSettings) {
        
        var printService = Components.classes["@mozilla.org/gfx/printsettings-service;1"]
                                     .getService(Components.interfaces.nsIPrintSettingsService);
        printService.savePrintSettingsToPrefs(printSettings, true, 
                                              printSettings.kInitSaveNativeData);
      }

    } catch (e) {
      dump("showPageSetup "+e+"\n");
      return false;
    }
    return true;
  },

  print: function ()
  {
    var webBrowserPrint = this.getWebBrowserPrint();
    var printSettings = this.getPrintSettings();
    try {
      webBrowserPrint.print(printSettings, null);
      if (gPrintSettingsAreGlobal && gSavePrintSettings) {
        var printService = Components.classes["@mozilla.org/gfx/printsettings-service;1"]
                                     .getService(Components.interfaces.nsIPrintSettingsService);
        printService.savePrintSettingsToPrefs(printSettings, true, 
                                              printSettings.kInitSaveAll);
        printService.savePrintSettingsToPrefs(printSettings, false,
                                              printSettings.kInitSavePrinterName);
      }
    } catch (e) {
      
      
      
      
    }
  },

  printPreview: function (aEnterPPCallback, aExitPPCallback)
  {
    
    
    
    var pptoolbar = document.getElementById("print-preview-toolbar");
    if (!pptoolbar) {
      this._onEnterPP = aEnterPPCallback;
      this._onExitPP  = aExitPPCallback;
    } else {
      
      
      
      var browser = getPPBrowser();
      if (browser)
        browser.collapsed = true;
    }

    this._webProgressPP = {};
    var ppParams        = {};
    var notifyOnOpen    = {};
    var webBrowserPrint = this.getWebBrowserPrint();
    var printSettings   = this.getPrintSettings();
    
    
    
    
    
    var printingPromptService = Components.classes["@mozilla.org/embedcomp/printingprompt-service;1"]
                                          .getService(Components.interfaces.nsIPrintingPromptService);
    
    
    try {
      printingPromptService.showProgress(this, webBrowserPrint, printSettings, this._obsPP, false,
                                         this._webProgressPP, ppParams, notifyOnOpen);
      if (ppParams.value) {
        var webNav = getWebNavigation();
        ppParams.value.docTitle = webNav.document.title;
        ppParams.value.docURL   = webNav.currentURI.spec;
      }

      
      
      if (!notifyOnOpen.value.valueOf() || this._webProgressPP.value == null)
        this.enterPrintPreview();
    } catch (e) {
      this.enterPrintPreview();
    }
  },

  getWebBrowserPrint: function ()
  {
    return getEngineWebBrowserPrint();
  },

  
  
  

  setPrinterDefaultsForSelectedPrinter: function (aPrintService, aPrintSettings)
  {
    if (!aPrintSettings.printerName)
      aPrintSettings.printerName = aPrintService.defaultPrinterName;

    
    aPrintService.initPrintSettingsFromPrinter(aPrintSettings.printerName, aPrintSettings);
    
    aPrintService.initPrintSettingsFromPrefs(aPrintSettings, true, aPrintSettings.kInitSaveAll);
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
      var printService = Components.classes["@mozilla.org/gfx/printsettings-service;1"]
                                   .getService(Components.interfaces.nsIPrintSettingsService);
      if (gPrintSettingsAreGlobal) {
        printSettings = printService.globalPrintSettings;
        this.setPrinterDefaultsForSelectedPrinter(printService, printSettings);
      } else {
        printSettings = printService.newPrintSettings;
      }
    } catch (e) {
      dump("getPrintSettings: "+e+"\n");
    }
    return printSettings;
  },

  _chromeState: {},
  _closeHandlerPP: null,
  _webProgressPP: null,
  _onEnterPP: null,
  _onExitPP: null,

  
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
    var webBrowserPrint = this.getWebBrowserPrint();
    var printSettings   = this.getPrintSettings();
    try {
      webBrowserPrint.printPreview(printSettings, null, this._webProgressPP.value);
    } catch (e) {
      
      
      
      
      return;
    }

    var printPreviewTB = document.getElementById("print-preview-toolbar");
    if (printPreviewTB) {
      printPreviewTB.updateToolbar();
      var browser = getPPBrowser();
      if (browser)
        browser.collapsed = false;
      return;
    }

    
    
    const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    printPreviewTB = document.createElementNS(XUL_NS, "toolbar");
    printPreviewTB.setAttribute("printpreview", true);
    printPreviewTB.setAttribute("id", "print-preview-toolbar");

    var navToolbox = getNavToolbox();
    navToolbox.parentNode.insertBefore(printPreviewTB, navToolbox);

    
    if (document.documentElement.hasAttribute("onclose"))
      this._closeHandlerPP = document.documentElement.getAttribute("onclose");
    else
      this._closeHandlerPP = null;
    document.documentElement.setAttribute("onclose", "PrintUtils.exitPrintPreview(); return false;");

    
    window.addEventListener("keypress", this.onKeyPressPP, true);
 
    
    navToolbox.focus();

    
    if (this._onEnterPP) {
      this._onEnterPP();
      this._onEnterPP = null;
    }
  },

  exitPrintPreview: function ()
  {
    window.removeEventListener("keypress", this.onKeyPressPP, true);

    
    document.documentElement.setAttribute("onclose", this._closeHandlerPP);
    this._closeHandlerPP = null;

    var webBrowserPrint = this.getWebBrowserPrint();
    webBrowserPrint.exitPrintPreview(); 

    
    var navToolbox = getNavToolbox();
    var printPreviewTB = document.getElementById("print-preview-toolbar");
    navToolbox.parentNode.removeChild(printPreviewTB);

    
    navToolbox.focus();

    
    if (this._onExitPP) {
      this._onExitPP();
      this._onExitPP = null;
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
    
    if (aEvent.keyCode == aEvent.DOM_VK_ESCAPE || isModif &&
        (aEvent.charCode == closeKey || aEvent.charCode == closeKey + 32))
      PrintUtils.exitPrintPreview();
    
    if (isModif) {
      aEvent.preventDefault();
      aEvent.stopPropagation();
    }
  }
}

function NSPrintSetup()
{
  PrintUtils.showPageSetup();
}

function NSPrint()
{
  PrintUtils.print();
}
