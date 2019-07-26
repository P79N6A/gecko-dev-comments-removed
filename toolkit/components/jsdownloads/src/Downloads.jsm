









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
XPCOMUtils.defineLazyModuleGetter(this, "DownloadSummary",
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

  



























  startDirect: function (aSource, aTarget, aOptions) {
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
    if (!this._promiseListsInitialized) {
      this._promiseListsInitialized = Task.spawn(function () {
        let publicList = new DownloadList();
        let privateList = new DownloadList();
        let combinedList = new DownloadCombinedList(publicList, privateList);

        try {
          yield DownloadIntegration.addListObservers(publicList, false);
          yield DownloadIntegration.addListObservers(privateList, true);
          yield DownloadIntegration.initializePublicDownloadList(publicList);
        } catch (ex) {
          Cu.reportError(ex);
        }

        let publicSummary = yield this.getSummary(Downloads.PUBLIC);
        let privateSummary = yield this.getSummary(Downloads.PRIVATE);
        let combinedSummary = yield this.getSummary(Downloads.ALL);

        yield publicSummary.bindToList(publicList);
        yield privateSummary.bindToList(privateList);
        yield combinedSummary.bindToList(combinedList);

        this._lists[Downloads.PUBLIC] = publicList;
        this._lists[Downloads.PRIVATE] = privateList;
        this._lists[Downloads.ALL] = combinedList;
      }.bind(this));
    }

    return this._promiseListsInitialized.then(() => this._lists[aType]);
  },

  



  _promiseListsInitialized: null,

  




  _lists: {},

  















  getSummary: function (aType)
  {
    if (aType != Downloads.PUBLIC && aType != Downloads.PRIVATE &&
        aType != Downloads.ALL) {
      throw new Error("Invalid aType argument.");
    }

    if (!(aType in this._summaries)) {
      this._summaries[aType] = new DownloadSummary();
    }

    return Promise.resolve(this._summaries[aType]);
  },

  




  _summaries: {},

  















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
