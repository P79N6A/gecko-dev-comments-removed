















"use strict";




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Downloads",
                                  "resource://gre/modules/Downloads.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");
































function DownloadLegacyTransfer()
{
  this._deferDownload = Promise.defer();
}

DownloadLegacyTransfer.prototype = {
  classID: Components.ID("{1b4c85df-cbdd-4bb6-b04e-613caece083c}"),

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsIWebProgressListener2,
                                         Ci.nsITransfer]),

  
  

  onStateChange: function DLT_onStateChange(aWebProgress, aRequest, aStateFlags,
                                            aStatus)
  {
    if (!Components.isSuccessCode(aStatus)) {
      this._componentFailed = true;
    }

    if ((aStateFlags & Ci.nsIWebProgressListener.STATE_START) &&
        (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK)) {
      
      
      this._deferDownload.promise.then(function (aDownload) {
        aDownload.saver.onTransferStarted(aRequest);
      }).then(null, Cu.reportError);
    } else if ((aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) &&
        (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK)) {
      
      
      this._deferDownload.promise.then(function DLT_OSC_onDownload(aDownload) {
        aDownload.saver.onTransferFinished(aRequest, aStatus);
      }).then(null, Cu.reportError);
    }
  },

  onProgressChange: function DLT_onProgressChange(aWebProgress, aRequest,
                                                  aCurSelfProgress,
                                                  aMaxSelfProgress,
                                                  aCurTotalProgress,
                                                  aMaxTotalProgress)
  {
    return onProgressChange64(aWebProgress, aRequest, aCurSelfProgress,
                              aMaxSelfProgress, aCurTotalProgress,
                              aMaxTotalProgress);
  },

  onLocationChange: function () { },

  onStatusChange: function DLT_onStatusChange(aWebProgress, aRequest, aStatus,
                                              aMessage)
  {
    
    
    
    if (!Components.isSuccessCode(aStatus)) {
      this._componentFailed = true;

      
      this._deferDownload.promise.then(function DLT_OSC_onDownload(aDownload) {
        aDownload.saver.onTransferFinished(aRequest, aStatus);
      }).then(null, Cu.reportError);
    }
  },

  onSecurityChange: function () { },

  
  

  onProgressChange64: function DLT_onProgressChange64(aWebProgress, aRequest,
                                                      aCurSelfProgress,
                                                      aMaxSelfProgress,
                                                      aCurTotalProgress,
                                                      aMaxTotalProgress)
  {
    
    this._deferDownload.promise.then(function DLT_OPC64_onDownload(aDownload) {
      aDownload.saver.onProgressBytes(aCurTotalProgress, aMaxTotalProgress);
    }).then(null, Cu.reportError);
  },

  onRefreshAttempted: function DLT_onRefreshAttempted(aWebProgress, aRefreshURI,
                                                      aMillis, aSameURI)
  {
    
    
    return true;
  },

  
  

  init: function DLT_init(aSource, aTarget, aDisplayName, aMIMEInfo, aStartTime,
                          aTempFile, aCancelable, aIsPrivate)
  {

    let launchWhenSuccedded = false, contentType = null, launcherPath = null;

    if (aMIMEInfo instanceof Ci.nsIMIMEInfo) {
      launchWhenSuccedded = aMIMEInfo.preferredAction != Ci.nsIMIMEInfo.saveToDisk;
      contentType = aMIMEInfo.type;
      let appHandler = aMIMEInfo.preferredApplicationHandler;

      if (appHandler instanceof Ci.nsILocalHandlerApp) {
        launcherPath = localHandler.executable.path;
      }
    }

    
    
    
    Downloads.createDownload({
      source: { url: aSource.spec, isPrivate: aIsPrivate },
      target: { path: aTarget.QueryInterface(Ci.nsIFileURL).file.path,
                partFilePath: aTempFile && aTempFile.path },
      saver: "legacy",
      launchWhenSuccedded: launchWhenSuccedded,
      contentType: contentType,
      launcherPath: launcherPath
    }).then(function DLT_I_onDownload(aDownload) {
      
      aDownload.saver.deferCanceled.promise.then(() => {
        
        if (!this._componentFailed) {
          aCancelable.cancel(Cr.NS_ERROR_ABORT);
        }
      }).then(null, Cu.reportError);

      
      if (aTempFile) {
        aDownload.tryToKeepPartialData = true;
      }

      
      aDownload.start().then(null, function () {
        
        aDownload.saver.deferCanceled.resolve();
      });

      
      this._deferDownload.resolve(aDownload);

      
      let list;
      if (aIsPrivate) {
        list = Downloads.getPrivateDownloadList();
      } else {
        list = Downloads.getPublicDownloadList();
      }
      return list.then(function (aList) aList.add(aDownload));
    }.bind(this)).then(null, Cu.reportError);
  },

  setSha256Hash: function () { },

  
  

  



  _deferDownload: null,

  




  _componentFailed: false,
};




this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DownloadLegacyTransfer]);
