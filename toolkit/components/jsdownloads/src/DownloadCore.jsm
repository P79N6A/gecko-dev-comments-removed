

































"use strict";

this.EXPORTED_SYMBOLS = [
  "Download",
  "DownloadSource",
  "DownloadTarget",
  "DownloadError",
  "DownloadSaver",
  "DownloadCopySaver",
  "DownloadLegacySaver",
];




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm")
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");

const BackgroundFileSaverStreamListener = Components.Constructor(
      "@mozilla.org/network/background-file-saver;1?mode=streamlistener",
      "nsIBackgroundFileSaver");









function Download()
{
  this._deferSucceeded = Promise.defer();
}

Download.prototype = {
  


  source: null,

  


  target: null,

  


  saver: null,

  





  stopped: true,

  


  succeeded: false,

  








  canceled: false,

  





  error: null,

  




  startTime: null,

  




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

  











  _currentAttempt: null,

  



















  start: function D_start()
  {
    
    if (this.succeeded) {
      return Promise.resolve();
    }

    
    
    
    if (this._currentAttempt) {
      return this._currentAttempt;
    }

    
    this.stopped = false;
    this.canceled = false;
    this.error = null;
    this.hasProgress = false;
    this.progress = 0;
    this.totalBytes = 0;
    this.currentBytes = 0;
    this.startTime = new Date();

    
    
    let deferAttempt = Promise.defer();
    let currentAttempt = deferAttempt.promise;
    this._currentAttempt = currentAttempt;

    
    
    function DS_setProgressBytes(aCurrentBytes, aTotalBytes)
    {
      if (this._currentAttempt == currentAttempt || !this._currentAttempt) {
        this._setBytes(aCurrentBytes, aTotalBytes);
      }
    }

    
    
    deferAttempt.resolve(Task.spawn(function task_D_start() {
      
      if (this._promiseCanceled) {
        yield this._promiseCanceled;
      }

      try {
        
        yield this.saver.execute(DS_setProgressBytes.bind(this));

        
        this.progress = 100;
        this.succeeded = true;
      } catch (ex) {
        
        
        
        if (this._promiseCanceled) {
          throw new DownloadError(Cr.NS_ERROR_FAILURE, "Download canceled.");
        }

        
        
        if (this._currentAttempt == currentAttempt || !this._currentAttempt) {
          this.error = ex;
        }
        throw ex;
      } finally {
        
        this._promiseCanceled = null;

        
        if (this._currentAttempt == currentAttempt || !this._currentAttempt) {
          this._currentAttempt = null;
          this.stopped = true;
          this._notifyChange();
          if (this.succeeded) {
            this._deferSucceeded.resolve();
          }
        }
      }
    }.bind(this)));

    
    this._notifyChange();
    return this._currentAttempt;
  },

  




  _promiseCanceled: null,

  





















  cancel: function D_cancel()
  {
    
    if (this.stopped) {
      return Promise.resolve();
    }

    if (!this._promiseCanceled) {
      
      let deferCanceled = Promise.defer();
      this._currentAttempt.then(function () deferCanceled.resolve(),
                                function () deferCanceled.resolve());
      this._promiseCanceled = deferCanceled.promise;

      
      this._currentAttempt = null;

      
      this.canceled = true;
      this._notifyChange();

      
      this.saver.cancel();
    }

    return this._promiseCanceled;
  },

  




  _deferSucceeded: null,

  












  whenSucceeded: function D_whenSucceeded()
  {
    return this._deferSucceeded.promise;
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

  












  execute: function DS_execute(aSetProgressBytesFn)
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

  


  execute: function DCS_execute(aSetProgressBytesFn)
  {
    let deferred = Promise.defer();
    let download = this.download;

    
    let backgroundFileSaver = new BackgroundFileSaverStreamListener();
    try {
      
      
      backgroundFileSaver.observer = {
        onTargetChange: function () { },
        onSaveComplete: function DCSE_onSaveComplete(aSaver, aStatus)
        {
          
          backgroundFileSaver.observer = null;
          this._backgroundFileSaver = null;

          
          if (Components.isSuccessCode(aStatus)) {
            deferred.resolve();
          } else {
            
            
            deferred.reject(new DownloadError(aStatus, null, true));
          }
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
          aSetProgressBytesFn(aProgress, aProgressMax);
        },
        onStatus: function () { },
      };

      
      backgroundFileSaver.QueryInterface(Ci.nsIStreamListener);
      channel.asyncOpen({
        onStartRequest: function DCSE_onStartRequest(aRequest, aContext)
        {
          backgroundFileSaver.onStartRequest(aRequest, aContext);

          
          
          if (aRequest instanceof Ci.nsIChannel &&
              aRequest.contentLength >= 0) {
            aSetProgressBytesFn(0, aRequest.contentLength);
          }
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









function DownloadLegacySaver()
{
  this.deferExecuted = Promise.defer();
  this.deferCanceled = Promise.defer();
}

DownloadLegacySaver.prototype = {
  __proto__: DownloadSaver.prototype,

  




  request: null,

  




  deferExecuted: null,

  





  deferCanceled: null,

  



  setProgressBytesFn: null,

  







  onProgressBytes: function DLS_onProgressBytes(aCurrentBytes, aTotalBytes)
  {
    
    if (!this.setProgressBytesFn) {
      return;
    }

    this.progressWasNotified = true;
    this.setProgressBytesFn(aCurrentBytes, aTotalBytes);
  },

  


  progressWasNotified: false,

  







  onTransferFinished: function DLS_onTransferFinished(aRequest, aStatus)
  {
    
    this.request = aRequest;

    if (Components.isSuccessCode(aStatus)) {
      this.deferExecuted.resolve();
    } else {
      
      
      this.deferExecuted.reject(new DownloadError(aStatus, null, true));
    }
  },

  


  execute: function DLS_execute(aSetProgressBytesFn)
  {
    this.setProgressBytesFn = aSetProgressBytesFn;

    return Task.spawn(function task_DLS_execute() {
      try {
        
        yield this.deferExecuted.promise;

        
        
        
        if (!this.progressWasNotified &&
            this.request instanceof Ci.nsIChannel &&
            this.request.contentLength >= 0) {
          aSetProgressBytesFn(0, this.request.contentLength);
        }

        
        
        
        try {
          
          let file = yield OS.File.open(this.download.target.file.path,
                                        { create: true });
          yield file.close();
        } catch (ex if ex instanceof OS.File.Error && ex.becauseExists) { }
      } finally {
        
        this.request = null;
      }
    }.bind(this));
  },

  


  cancel: function DLS_cancel()
  {
    
    this.deferCanceled.resolve();

    
    
    
    this.deferExecuted.reject(new DownloadError(Cr.NS_ERROR_FAILURE,
                                                "Download canceled."));
  },
};
