









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
XPCOMUtils.defineLazyModuleGetter(this, "DownloadUIHelper",
                                  "resource://gre/modules/DownloadUIHelper.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");








this.Downloads = {
  

































  createDownload: function D_createDownload(aProperties)
  {
    try {
      return Promise.resolve(Download.fromSerializable(aProperties));
    } catch (ex) {
      return Promise.reject(ex);
    }
  },

  
























  simpleDownload: function D_simpleDownload(aSource, aTarget, aOptions) {
    return this.createDownload({
      source: aSource,
      target: aTarget,
    }).then(function D_SD_onSuccess(aDownload) {
      if (aOptions && ("isPrivate" in aOptions)) {
        aDownload.source.isPrivate = aOptions.isPrivate;
      }
      return aDownload.start();
    });
  },

  












  getPublicDownloadList: function D_getPublicDownloadList()
  {
    if (!this._promisePublicDownloadList) {
      this._promisePublicDownloadList = Task.spawn(
        function task_D_getPublicDownloadList() {
          let list = new DownloadList(true);
          try {
            yield DownloadIntegration.loadPersistent(list);
          } catch (ex) {
            Cu.reportError(ex);
          }
          throw new Task.Result(list);
        });
    }
    return this._promisePublicDownloadList;
  },

  




  _promisePublicDownloadList: null,

  









  getPrivateDownloadList: function D_getPrivateDownloadList()
  {
    if (!this._privateDownloadList) {
      this._privateDownloadList = new DownloadList(false);
    }
    return Promise.resolve(this._privateDownloadList);
  },
  _privateDownloadList: null,

  















  getSystemDownloadsDirectory: function D_getSystemDownloadsDirectory() {
    return DownloadIntegration.getSystemDownloadsDirectory();
  },

  






  getUserDownloadsDirectory: function D_getUserDownloadsDirectory() {
    return DownloadIntegration.getUserDownloadsDirectory();
  },

  








  getTemporaryDownloadsDirectory: function D_getTemporaryDownloadsDirectory() {
    return DownloadIntegration.getTemporaryDownloadsDirectory();
  },

  




  Error: DownloadError,
};
