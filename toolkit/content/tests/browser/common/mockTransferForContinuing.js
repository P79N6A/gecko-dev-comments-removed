



































Cu.import("resource://gre/modules/XPCOMUtils.jsm");






function MockTransferForContinuing()
{
  this._downloadIsSuccessful = true;
}

MockTransferForContinuing.prototype = {
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIWebProgressListener,
    Ci.nsIWebProgressListener2,
    Ci.nsITransfer,
  ]),

  
  

  onStateChange: function MTFC_onStateChange(aWebProgress, aRequest,
                                             aStateFlags, aStatus) {
    
    if (!Components.isSuccessCode(aStatus))
      this._downloadIsSuccessful = false;

    
    if ((aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) &&
        (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK))
      
      testRunner.continueTest(this._downloadIsSuccessful);
  },
  onProgressChange: function(aWebProgress, aRequest, aCurSelfProgress,
                             aMaxSelfProgress, aCurTotalProgress,
                             aMaxTotalProgress) { },
  onLocationChange: function(aWebProgress, aRequest, aLocation) { },
  onStatusChange: function MTFC_onStatusChange(aWebProgress, aRequest, aStatus,
                                               aMessage) {
    
    if (!Components.isSuccessCode(aStatus))
      this._downloadIsSuccessful = false;
  },
  onSecurityChange: function(aWebProgress, aRequest, aState) { },

  
  

  onProgressChange64: function(aWebProgress, aRequest, aCurSelfProgress,
                               aMaxSelfProgress, aCurTotalProgress,
                               aMaxTotalProgress) { },
  onRefreshAttempted: function(aWebProgress, aRefreshURI, aMillis,
                               aSameURI) { },

  
  

  init: function(aSource, aTarget, aDisplayName, aMIMEInfo, aStartTime,
                 aTempFile, aCancelable) { }
};







var mockTransferForContinuingRegisterer =
  new MockObjectRegisterer("@mozilla.org/transfer;1",
                           MockTransferForContinuing);
