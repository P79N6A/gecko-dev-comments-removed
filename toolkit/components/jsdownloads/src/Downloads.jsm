









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

XPCOMUtils.defineLazyModuleGetter(this, "DownloadCombinedList",
                                  "resource://gre/modules/DownloadList.jsm");
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
  


  get PUBLIC() "{Downloads.PUBLIC}",
  


  get PRIVATE() "{Downloads.PRIVATE}",
  


  get ALL() "{Downloads.ALL}",

  

































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

  


















  getList: function (aType)
  {
    if (aType != Downloads.PUBLIC && aType != Downloads.PRIVATE &&
        aType != Downloads.ALL) {
      throw new Error("Invalid aType argument.");
    }

    if (!(aType in this._listPromises)) {
      this._listPromises[aType] = Task.spawn(function () {
        let list;
        if (aType == Downloads.ALL) {
          list = new DownloadCombinedList(
                       (yield this.getList(Downloads.PUBLIC)),
                       (yield this.getList(Downloads.PRIVATE)));
        } else {
          list = new DownloadList();
          try {
            yield DownloadIntegration.addListObservers(
                                        list, aType == Downloads.PRIVATE);
            if (aType == Downloads.PUBLIC) {
              yield DownloadIntegration.initializePublicDownloadList(list);
            }
          } catch (ex) {
            Cu.reportError(ex);
          }
        }
        throw new Task.Result(list);
      }.bind(this));
    }

    return this._listPromises[aType];
  },

  




  _listPromises: {},

  















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
