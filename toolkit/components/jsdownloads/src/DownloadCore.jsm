



























"use strict";

this.EXPORTED_SYMBOLS = [
  "Download",
  "DownloadSource",
  "DownloadTarget",
  "DownloadSaver",
  "DownloadCopySaver",
];




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/promise/core.js");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");









function Download()
{
  this._deferDone = Promise.defer();
}

Download.prototype = {
  


  source: null,

  


  target: null,

  


  saver: null,

  



  _deferDone: null,

  






  start: function D_start()
  {
    this._deferDone.resolve(Task.spawn(function task_D_start() {
      yield this.saver.execute();
    }.bind(this)));

    return this.whenDone();
  },

  






  whenDone: function D_whenDone() {
    return this._deferDone.promise;
  },
};







function DownloadSource() { }

DownloadSource.prototype = {
  


  uri: null,
};








function DownloadTarget() { }

DownloadTarget.prototype = {
  


  file: null,
};







function DownloadSaver() { }

DownloadSaver.prototype = {
  


  download: null,

  






  execute: function DS_execute()
  {
    throw new Error("Not implemented.");
  }
};







function DownloadCopySaver() { }

DownloadCopySaver.prototype = {
  __proto__: DownloadSaver,

  


  execute: function DCS_execute()
  {
    let deferred = Promise.defer();

    NetUtil.asyncFetch(this.download.source.uri, function (aInputStream, aResult) {
      if (!Components.isSuccessCode(aResult)) {
        deferred.reject(new Components.Exception("Download failed.", aResult));
        return;
      }

      let fileOutputStream = Cc["@mozilla.org/network/file-output-stream;1"]
                              .createInstance(Ci.nsIFileOutputStream);
      fileOutputStream.init(this.download.target.file, -1, -1, 0);

      NetUtil.asyncCopy(aInputStream, fileOutputStream, function (aResult) {
        if (!Components.isSuccessCode(aResult)) {
          deferred.reject(new Components.Exception("Download failed.", aResult));
          return;
        }

        deferred.resolve();
      }.bind(this));
    }.bind(this));

    return deferred.promise;
  },
};
