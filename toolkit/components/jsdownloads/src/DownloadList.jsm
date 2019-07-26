










"use strict";

this.EXPORTED_SYMBOLS = [
  "DownloadList",
];




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");








function DownloadList() {
  this._downloads = [];
  this._views = new Set();
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
};
