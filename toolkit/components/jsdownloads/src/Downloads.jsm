









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
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
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
      if ("isPrivate" in aProperties.source) {
        download.source.isPrivate = aProperties.source.isPrivate;
      }
      if ("referrer" in aProperties.source) {
        download.source.referrer = aProperties.source.referrer;
      }
      download.target = new DownloadTarget();
      download.target.file = aProperties.target.file;

      
      download.saver = aProperties.saver.type == "legacy"
                       ? new DownloadLegacySaver()
                       : new DownloadCopySaver();
      download.saver.download = download;

      
      
      yield;
      throw new Task.Result(download);
    });
  },

  






















  simpleDownload: function D_simpleDownload(aSource, aTarget, aOptions) {
    
    
    if (aSource instanceof Ci.nsIURI) {
      aSource = { uri: aSource };
    } else if (typeof aSource == "string" ||
               (typeof aSource == "object" && "charAt" in aSource)) {
      aSource = { uri: NetUtil.newURI(aSource) };
    }

    if (aSource && aOptions && ("isPrivate" in aOptions)) {
      aSource.isPrivate = aOptions.isPrivate;
    }
    if (aTarget instanceof Ci.nsIFile) {
      aTarget = { file: aTarget };
    } else if (typeof aTarget == "string" ||
               (typeof aTarget == "object" && "charAt" in aTarget)) {
      aTarget = { file: new FileUtils.File(aTarget) };
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
      this._publicDownloadList = new DownloadList(true);
    }
    return Promise.resolve(this._publicDownloadList);
  },
  _publicDownloadList: null,

  









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
