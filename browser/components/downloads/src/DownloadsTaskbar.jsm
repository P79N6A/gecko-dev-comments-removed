









"use strict";

this.EXPORTED_SYMBOLS = [
  "DownloadsTaskbar",
];




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Downloads",
                                  "resource://gre/modules/Downloads.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "RecentWindow",
                                  "resource:///modules/RecentWindow.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "gWinTaskbar", function () {
  if (!("@mozilla.org/windows-taskbar;1" in Cc)) {
    return null;
  }
  let winTaskbar = Cc["@mozilla.org/windows-taskbar;1"]
                     .getService(Ci.nsIWinTaskbar);
  return winTaskbar.available && winTaskbar;
});

XPCOMUtils.defineLazyGetter(this, "gMacTaskbarProgress", function () {
  return ("@mozilla.org/widget/macdocksupport;1" in Cc) &&
         Cc["@mozilla.org/widget/macdocksupport;1"]
           .getService(Ci.nsITaskbarProgress);
});







this.DownloadsTaskbar = {
  



  _summary: null,

  




  _taskbarProgress: null,

  















  registerIndicator: function (aBrowserWindow)
  {
    if (!this._taskbarProgress) {
      if (gMacTaskbarProgress) {
        
        this._taskbarProgress = gMacTaskbarProgress;
        
        Services.obs.addObserver(() => {
          this._taskbarProgress = null;
          gMacTaskbarProgress = null;
        }, "quit-application-granted", false);
      } else if (gWinTaskbar) {
        
        
        this._attachIndicator(aBrowserWindow);
      } else {
        
        return;
      }
    }

    
    if (!this._summary) {
      Downloads.getSummary(Downloads.ALL).then(summary => {
        
        
        if (this._summary) {
          return;
        }
        this._summary = summary;
        return this._summary.addView(this);
      }).then(null, Cu.reportError);
    }
  },

  


  _attachIndicator: function (aWindow)
  {
    
    let docShell = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIWebNavigation)
                          .QueryInterface(Ci.nsIDocShellTreeItem).treeOwner
                          .QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIXULWindow).docShell;
    this._taskbarProgress = gWinTaskbar.getTaskbarProgress(docShell);

    
    
    
    if (this._summary) {
      this.onSummaryChanged();
    }

    aWindow.addEventListener("unload", () => {
      
      let browserWindow = RecentWindow.getMostRecentBrowserWindow();
      if (browserWindow) {
        
        this._attachIndicator(browserWindow);
      } else {
        
        
        
        this._taskbarProgress = null;
      }
    }, false);
  },

  
  

  onSummaryChanged: function ()
  {
    
    if (!this._taskbarProgress) {
      return;
    }

    if (this._summary.allHaveStopped || this._summary.progressTotalBytes == 0) {
      this._taskbarProgress.setProgressState(
                               Ci.nsITaskbarProgress.STATE_NO_PROGRESS, 0, 0);
    } else {
      
      
      
      let progressCurrentBytes = Math.min(this._summary.progressTotalBytes,
                                          this._summary.progressCurrentBytes);
      this._taskbarProgress.setProgressState(
                               Ci.nsITaskbarProgress.STATE_NORMAL,
                               progressCurrentBytes,
                               this._summary.progressTotalBytes);
    }
  },
};
