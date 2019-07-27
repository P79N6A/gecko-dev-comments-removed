










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
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
                                  "resource://gre/modules/PrivateBrowsingUtils.jsm");




function DownloadsUI()
{
}

DownloadsUI.prototype = {
  classID: Components.ID("{4d99321e-d156-455b-81f7-e7aa2308134f}"),

  _xpcom_factory: XPCOMUtils.generateSingletonFactory(DownloadsUI),

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDownloadManagerUI]),

  
  

  show: function DUI_show(aWindowContext, aDownload, aReason, aUsePrivateUI)
  {
    if (!aReason) {
      aReason = Ci.nsIDownloadManagerUI.REASON_USER_INTERACTED;
    }

    if (aReason == Ci.nsIDownloadManagerUI.REASON_NEW_DOWNLOAD) {
      const kMinimized = Ci.nsIDOMChromeWindow.STATE_MINIMIZED;
      let browserWin = gBrowserGlue.getMostRecentBrowserWindow();

      if (!browserWin || browserWin.windowState == kMinimized) {
        this._showDownloadManagerUI(aWindowContext, aUsePrivateUI);
      }
      else {
        
        
        browserWin.DownloadsButton.checkIsVisible(function(isVisible) {
          if (!isVisible) {
            this._showDownloadManagerUI(aWindowContext, aUsePrivateUI);
          }
        }.bind(this));
      }
    } else {
      this._showDownloadManagerUI(aWindowContext, aUsePrivateUI);
    }
  },

  get visible() true,

  getAttention: function () {},

  
  

  


  _showDownloadManagerUI: function (aWindowContext, aUsePrivateUI)
  {
    
    
    let parentWindow = aWindowContext;
    if (!parentWindow) {
      parentWindow = RecentWindow.getMostRecentBrowserWindow({ private: !!aUsePrivateUI });
      if (!parentWindow) {
        Components.utils.reportError(
          "Couldn't find a browser window to open the Places Downloads View " +
          "from.");
        return;
      }
    }

    
    if (PrivateBrowsingUtils.isWindowPrivate(parentWindow)) {
      parentWindow.openUILinkIn("about:downloads", "tab");
      return;
    } else {
      let organizer = Services.wm.getMostRecentWindow("Places:Organizer");
      if (!organizer) {
        parentWindow.openDialog("chrome://browser/content/places/places.xul",
                                "", "chrome,toolbar=yes,dialog=no,resizable",
                                "Downloads");
      } else {
        organizer.PlacesOrganizer.selectLeftPaneQuery("Downloads");
        organizer.focus();
      }
    }
  }
};




this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DownloadsUI]);
