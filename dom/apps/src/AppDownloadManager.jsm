



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/FreeSpaceWatcher.jsm");

this.EXPORTED_SYMBOLS = ["AppDownloadManager"];

function debug(aMsg) {
  
}

this.AppDownloadManager = {
  
  
  MIN_REMAINING_FREESPACE: 5 * 1024 * 1024,

  downloads: {},
  count: 0,
  cancelFunc: null,
  timer: null,

  




  registerCancelFunction: function app_dlMgr_registerCancel(aFunction) {
    this.cancelFunc = aFunction;
  },

  




  add: function app_dlMgr_add(aManifestURL, aDownload) {
    debug("Adding " + aManifestURL);
    if (!(aManifestURL in this.downloads)) {
      this.count++;
      if (this.count == 1) {
        this.timer = FreeSpaceWatcher.create(this.MIN_REMAINING_FREESPACE,
                                             this._spaceWatcher.bind(this));
      }
    }
    this.downloads[aManifestURL] = aDownload;
  },

  




  get: function app_dlMgr_get(aManifestURL) {
    debug("Getting " + aManifestURL);
    return this.downloads[aManifestURL];
  },

  



  remove: function app_dlMgr_remove(aManifestURL) {
    debug("Removing " + aManifestURL);
    if (aManifestURL in this.downloads) {
      this.count--;
      delete this.downloads[aManifestURL];
      if (this.count == 0) {
        FreeSpaceWatcher.stop(this.timer);
      }
    }
  },

  



  _spaceWatcher: function app_dlMgr_watcher(aStatus) {
    debug("Disk space is now " + aStatus);
    if (aStatus == "free") {
      
      return;
    }

    
    
    
    for (let url in this.downloads) {
      this.cancelFunc(url, "INSUFFICIENT_STORAGE");
    }
  }
}
