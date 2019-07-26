













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
XPCOMUtils.defineLazyModuleGetter(this, "RecentWindow",
                                  "resource:///modules/RecentWindow.jsm");




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
      const kMinimized = Ci.nsIDOMChromeWindow.STATE_MINIMIZED;
      let browserWin = gBrowserGlue.getMostRecentBrowserWindow();

      if (!browserWin || browserWin.windowState == kMinimized) {
        this._showDownloadManagerUI(aWindowContext, aID, aReason);
      }
      else {
        
        
        browserWin.DownloadsButton.checkIsVisible(function(isVisible) {
          if (!isVisible) {
            this._showDownloadManagerUI(aWindowContext, aID, aReason);
          }
        }.bind(this));
      }
    } else {
      this._showDownloadManagerUI(aWindowContext, aID, aReason);
    }
  },

  get visible()
  {
    
    
    
    
    return DownloadsCommon.useToolkitUI ? this._toolkitUI.visible : true;
  },

  getAttention: function DUI_getAttention()
  {
    if (DownloadsCommon.useToolkitUI) {
      this._toolkitUI.getAttention();
    }
  },

  


  _showDownloadManagerUI:
  function DUI_showDownloadManagerUI(aWindowContext, aID, aReason)
  {
    let organizer = Services.wm.getMostRecentWindow("Places:Organizer");
    if (!organizer) {
      let parentWindow = aWindowContext;
      
      
      
      if (!parentWindow) {
        parentWindow = RecentWindow.getMostRecentBrowserWindow();
        if (!parentWindow) {
          Components.utils
                    .reportError("Couldn't find a browser window to open " +
                                 "the Places Downloads View from.");
          return;
        }
      }
      parentWindow.openDialog("chrome://browser/content/places/places.xul",
                              "", "chrome,toolbar=yes,dialog=no,resizable",
                              "Downloads");
    }
    else {
      organizer.PlacesOrganizer.selectLeftPaneQuery("Downloads");
      organizer.focus();
    }
  }
};




this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DownloadsUI]);
