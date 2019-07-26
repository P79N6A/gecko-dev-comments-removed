










"use strict";

this.EXPORTED_SYMBOLS = [
  "DownloadList",
];




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");











function DownloadList(aIsPublic) {
  this._downloads = [];
  this._views = new Set();
  
  
  if (aIsPublic) {
    PlacesUtils.history.addObserver(this, false);
  }
}

DownloadList.prototype = {
  


  _downloads: null,

  








  getAll: function DL_getAll() {
    return Promise.resolve(Array.slice(this._downloads, 0));
  },

  











  add: function DL_add(aDownload) {
    this._downloads.push(aDownload);
    aDownload.onchange = this._change.bind(this, aDownload);

    for (let view of this._views) {
      try {
        if (view.onDownloadAdded) {
          view.onDownloadAdded(aDownload);
        }
      } catch (ex) {
        Cu.reportError(ex);
      }
    }
  },

  











  remove: function DL_remove(aDownload) {
    let index = this._downloads.indexOf(aDownload);
    if (index != -1) {
      this._downloads.splice(index, 1);
      aDownload.onchange = null;

      for (let view of this._views) {
        try {
          if (view.onDownloadRemoved) {
            view.onDownloadRemoved(aDownload);
          }
        } catch (ex) {
          Cu.reportError(ex);
        }
      }
    }
  },

  





  _change: function DL_change(aDownload) {
    for (let view of this._views) {
      try {
        if (view.onDownloadChanged) {
          view.onDownloadChanged(aDownload);
        }
      } catch (ex) {
        Cu.reportError(ex);
      }
    }
  },

  


  _views: null,

  


















  addView: function DL_addView(aView)
  {
    this._views.add(aView);

    if (aView.onDownloadAdded) {
      for (let download of this._downloads) {
        try {
          aView.onDownloadAdded(download);
        } catch (ex) {
          Cu.reportError(ex);
        }
      }
    }
  },

  






  removeView: function DL_removeView(aView)
  {
    this._views.delete(aView);
  },

  





  _removeWhere: function DL__removeWhere(aTestFn) {
    Task.spawn(function() {
      let list = yield this.getAll();
      for (let download of list) {
        
        
        if ((download.succeeded || download.canceled || download.error) &&
            aTestFn(download)) {
          
          
          this.remove(download);
          
          
          
          
          download.finalize(true);
        }
      }
    }.bind(this)).then(null, Cu.reportError);
  },

  







  removeByTimeframe: function DL_removeByTimeframe(aStartTime, aEndTime) {
    this._removeWhere(download => download.startTime >= aStartTime &&
                                  download.startTime <= aEndTime);
  },

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsINavHistoryObserver]),

  
  

  onDeleteURI: function DL_onDeleteURI(aURI, aGUID) {
    this._removeWhere(download => aURI.equals(NetUtil.newURI(
                                                      download.source.url)));
  },

  onClearHistory: function DL_onClearHistory() {
    this._removeWhere(() => true);
  },

  onTitleChanged: function () {},
  onBeginUpdateBatch: function () {},
  onEndUpdateBatch: function () {},
  onVisit: function () {},
  onPageChanged: function () {},
  onDeleteVisits: function () {},
};

