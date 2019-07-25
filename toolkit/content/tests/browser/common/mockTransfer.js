



































Cc["@mozilla.org/moz/jssubscript-loader;1"]
  .getService(Ci.mozIJSSubScriptLoader)
  .loadSubScript("chrome://mochitests/content/browser/toolkit/content/tests/browser/common/mockObjects.js",
                 this);

var mockTransferCallback;






function MockTransfer() {
  this._downloadIsSuccessful = true;
}

MockTransfer.prototype = {
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
      
      mockTransferCallback(this._downloadIsSuccessful);
  },
  onProgressChange: function () {},
  onLocationChange: function () {},
  onStatusChange: function MTFC_onStatusChange(aWebProgress, aRequest, aStatus,
                                               aMessage) {
    
    if (!Components.isSuccessCode(aStatus))
      this._downloadIsSuccessful = false;
  },
  onSecurityChange: function () {},

  
  onProgressChange64: function () {},
  onRefreshAttempted: function () {},

  
  init: function () {}
};







var mockTransferRegisterer =
  new MockObjectRegisterer("@mozilla.org/transfer;1",  MockTransfer);
