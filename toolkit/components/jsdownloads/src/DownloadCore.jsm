






























"use strict";

this.EXPORTED_SYMBOLS = [
  "Download",
  "DownloadSource",
  "DownloadTarget",
  "DownloadError",
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
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");

const BackgroundFileSaverStreamListener = Components.Constructor(
      "@mozilla.org/network/background-file-saver;1?mode=streamlistener",
      "nsIBackgroundFileSaver");









function Download()
{
  this._deferStopped = Promise.defer();
}

Download.prototype = {
  


  source: null,

  


  target: null,

  


  saver: null,

  




  stopped: false,

  



  canceled: false,

  




  error: null,

  




  hasProgress: false,

  







  progress: 0,

  





  totalBytes: 0,

  







  currentBytes: 0,

  


  onchange: null,

  


  _notifyChange: function D_notifyChange() {
    try {
      if (this.onchange) {
        this.onchange();
      }
    } catch (ex) {
      Cu.reportError(ex);
    }
  },

  



  _deferStopped: null,

  






  start: function D_start()
  {
    this._deferStopped.resolve(Task.spawn(function task_D_start() {
      try {
        yield this.saver.execute();
        this.progress = 100;
      } catch (ex) {
        if (this.canceled) {
          throw new DownloadError(Cr.NS_ERROR_FAILURE, "Download canceled.");
        }
        this.error = ex;
        throw ex;
      } finally {
        this.stopped = true;
        this._notifyChange();
      }
    }.bind(this)));

    return this._deferStopped.promise;
  },

  


  cancel: function D_cancel()
  {
    if (this.stopped || this.canceled) {
      return;
    }

    this.canceled = true;
    this.saver.cancel();
  },

  







  _setBytes: function D_setBytes(aCurrentBytes, aTotalBytes) {
    this.currentBytes = aCurrentBytes;
    if (aTotalBytes != -1) {
      this.hasProgress = true;
      this.totalBytes = aTotalBytes;
      if (aTotalBytes > 0) {
        this.progress = Math.floor(aCurrentBytes / aTotalBytes * 100);
      }
    }
    this._notifyChange();
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


















function DownloadError(aResult, aMessage, aInferCause)
{
  const NS_ERROR_MODULE_BASE_OFFSET = 0x45;
  const NS_ERROR_MODULE_NETWORK = 6;
  const NS_ERROR_MODULE_FILES = 13;

  
  this.name = "DownloadError";
  this.result = aResult || Cr.NS_ERROR_FAILURE;
  if (aMessage) {
    this.message = aMessage;
  } else {
    let exception = new Components.Exception(this.result);
    this.message = exception.toString();
  }
  if (aInferCause) {
    let module = ((aResult & 0x7FFF0000) >> 16) - NS_ERROR_MODULE_BASE_OFFSET;
    this.becauseSourceFailed = (module == NS_ERROR_MODULE_NETWORK);
    this.becauseTargetFailed = (module == NS_ERROR_MODULE_FILES);
  }
}

DownloadError.prototype = {
  __proto__: Error.prototype,

  


  result: false,

  


  becauseSourceFailed: false,

  


  becauseTargetFailed: false,
};







function DownloadSaver() { }

DownloadSaver.prototype = {
  


  download: null,

  






  execute: function DS_execute()
  {
    throw new Error("Not implemented.");
  },

  


  cancel: function DS_cancel()
  {
    throw new Error("Not implemented.");
  },
};







function DownloadCopySaver() { }

DownloadCopySaver.prototype = {
  __proto__: DownloadSaver.prototype,

  


  _backgroundFileSaver: null,

  


  execute: function DCS_execute()
  {
    let deferred = Promise.defer();
    let download = this.download;

    
    let backgroundFileSaver = new BackgroundFileSaverStreamListener();
    try {
      
      
      backgroundFileSaver.observer = {
        onTargetChange: function () { },
        onSaveComplete: function DCSE_onSaveComplete(aSaver, aStatus)
        {
          if (Components.isSuccessCode(aStatus)) {
            deferred.resolve();
          } else {
            
            
            deferred.reject(new DownloadError(aStatus, null, true));
          }

          
          backgroundFileSaver.observer = null;
        },
      };

      
      backgroundFileSaver.setTarget(download.target.file, false);

      
      
      let channel = NetUtil.newChannel(download.source.uri);
      channel.notificationCallbacks = {
        QueryInterface: XPCOMUtils.generateQI([Ci.nsIInterfaceRequestor]),
        getInterface: XPCOMUtils.generateQI([Ci.nsIProgressEventSink]),
        onProgress: function DCSE_onProgress(aRequest, aContext, aProgress,
                                             aProgressMax)
        {
          download._setBytes(aProgress, aProgressMax);
        },
        onStatus: function () { },
      };

      
      backgroundFileSaver.QueryInterface(Ci.nsIStreamListener);
      channel.asyncOpen({
        onStartRequest: function DCSE_onStartRequest(aRequest, aContext)
        {
          backgroundFileSaver.onStartRequest(aRequest, aContext);
        },
        onStopRequest: function DCSE_onStopRequest(aRequest, aContext,
                                                   aStatusCode)
        {
          try {
            backgroundFileSaver.onStopRequest(aRequest, aContext, aStatusCode);
          } finally {
            
            
            
            if (Components.isSuccessCode(aStatusCode)) {
              backgroundFileSaver.finish(Cr.NS_OK);
            }
          }
        },
        onDataAvailable: function DCSE_onDataAvailable(aRequest, aContext,
                                                       aInputStream, aOffset,
                                                       aCount)
        {
          backgroundFileSaver.onDataAvailable(aRequest, aContext, aInputStream,
                                              aOffset, aCount);
        },
      }, null);

      
      this._backgroundFileSaver = backgroundFileSaver;
    } catch (ex) {
      
      
      deferred.reject(ex);
      backgroundFileSaver.finish(Cr.NS_ERROR_FAILURE);
    }
    return deferred.promise;
  },

  


  cancel: function DCS_cancel()
  {
    if (this._backgroundFileSaver) {
      this._backgroundFileSaver.finish(Cr.NS_ERROR_FAILURE);
      this._backgroundFileSaver = null;
    }
  },
};
