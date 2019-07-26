









"use strict";

this.EXPORTED_SYMBOLS = [
  "Downloads",
];




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/DownloadCore.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DownloadIntegration",
                                  "resource://gre/modules/DownloadIntegration.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadList",
                                  "resource://gre/modules/DownloadList.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadStore",
                                  "resource://gre/modules/DownloadStore.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadUIHelper",
                                  "resource://gre/modules/DownloadUIHelper.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");








this.Downloads = {
  





















  createDownload: function D_createDownload(aProperties)
  {
    return Task.spawn(function task_D_createDownload() {
      let download = new Download();

      download.source = new DownloadSource();
      download.source.uri = aProperties.source.uri;
      download.target = new DownloadTarget();
      download.target.file = aProperties.target.file;

      
      download.saver = new DownloadCopySaver();
      download.saver.download = download;

      
      
      yield;
      throw new Task.Result(download);
    });
  },

  















  simpleDownload: function D_simpleDownload(aSource, aTarget) {
    
    
    if (aSource instanceof Ci.nsIURI) {
      aSource = { uri: aSource };
    }
    if (aTarget instanceof Ci.nsIFile) {
      aTarget = { file: aTarget };
    }

    
    return this.createDownload({
      source: aSource,
      target: aTarget,
      saver: { type: "copy" },
    }).then(function D_SD_onSuccess(aDownload) {
      return aDownload.start();
    });
  },

  












  getPublicDownloadList: function D_getPublicDownloadList()
  {
    if (!this._publicDownloadList) {
      this._publicDownloadList = new DownloadList();
    }
    return Promise.resolve(this._publicDownloadList);
  },
  _publicDownloadList: null,

  




  Error: DownloadError,
};
