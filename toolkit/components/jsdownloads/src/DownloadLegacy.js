















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
    
    if ((aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) &&
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
    
    
    
    Downloads.createDownload({
      source: { uri: aSource },
      target: { file: aTarget.QueryInterface(Ci.nsIFileURL).file },
      saver: { type: "legacy" },
    }).then(function DLT_I_onDownload(aDownload) {
      
      aDownload.saver.deferCanceled.promise
                     .then(function () aCancelable.cancel(Cr.NS_ERROR_ABORT))
                     .then(null, Cu.reportError);

      
      aDownload.start();

      
      this._deferDownload.resolve(aDownload);

      
      return Downloads.getPublicDownloadList()
                      .then(function (aList) aList.add(aDownload));
    }.bind(this)).then(null, Cu.reportError);
  },

  
  

  



  _deferDownload: null,
};




this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DownloadLegacyTransfer]);
