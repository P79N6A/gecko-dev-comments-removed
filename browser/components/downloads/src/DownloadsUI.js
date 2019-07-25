













"use strict";




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DownloadsCommon",
                                  "resource:///modules/DownloadsCommon.jsm");
XPCOMUtils.defineLazyServiceGetter(this, "gBrowserGlue",
                                   "@mozilla.org/browser/browserglue;1",
                                   "nsIBrowserGlue");




function DownloadsUI()
{
  XPCOMUtils.defineLazyGetter(this, "_toolkitUI", function () {
    
    return Components.classesByID["{7dfdf0d1-aff6-4a34-bad1-d0fe74601642}"]
                     .getService(Ci.nsIDownloadManagerUI);
  });
}

DownloadsUI.prototype = {
  classID: Components.ID("{4d99321e-d156-455b-81f7-e7aa2308134f}"),

  _xpcom_factory: XPCOMUtils.generateSingletonFactory(DownloadsUI),

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDownloadManagerUI]),

  
  

  show: function DUI_show(aWindowContext, aID, aReason)
  {
    if (DownloadsCommon.useToolkitUI) {
      this._toolkitUI.show(aWindowContext, aID, aReason);
      return;
    }

    if (!aReason) {
      aReason = Ci.nsIDownloadManagerUI.REASON_USER_INTERACTED;
    }

    if (aReason == Ci.nsIDownloadManagerUI.REASON_NEW_DOWNLOAD) {
      
      
      
      
      return;
    }

    
    let browserWin = gBrowserGlue.getMostRecentBrowserWindow();
    if (browserWin) {
      browserWin.focus();
      browserWin.DownloadsPanel.showPanel();
      return;
    }

    
    
    
    Services.obs.addObserver(function DUIO_observe(aSubject, aTopic, aData) {
      Services.obs.removeObserver(DUIO_observe, aTopic);
      aSubject.DownloadsPanel.showPanel();
    }, "browser-delayed-startup-finished", false);

    
    let windowFirstArg = Cc["@mozilla.org/supports-string;1"]
                         .createInstance(Ci.nsISupportsString);
    let windowArgs = Cc["@mozilla.org/supports-array;1"]
                     .createInstance(Ci.nsISupportsArray);
    windowArgs.AppendElement(windowFirstArg);
    Services.ww.openWindow(null, "chrome://browser/content/browser.xul",
                           null, "chrome,dialog=no,all", windowArgs);
  },

  get visible()
  {
    if (DownloadsCommon.useToolkitUI) {
      return this._toolkitUI.visible;
    }

    let browserWin = gBrowserGlue.getMostRecentBrowserWindow();
    return browserWin ? browserWin.DownloadsPanel.isPanelShowing : false;
  },

  getAttention: function DUI_getAttention()
  {
    if (DownloadsCommon.useToolkitUI) {
      this._toolkitUI.getAttention();
    }
  }
};




const NSGetFactory = XPCOMUtils.generateNSGetFactory([DownloadsUI]);
