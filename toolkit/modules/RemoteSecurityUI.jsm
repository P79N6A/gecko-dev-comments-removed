




this.EXPORTED_SYMBOLS = ["RemoteSecurityUI"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function RemoteSecurityUI()
{
    this._state = 0;
    this._SSLStatus = null;
}

RemoteSecurityUI.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISSLStatusProvider, Ci.nsISecureBrowserUI]),

  
  get state() { return this._state; },
  get tooltipText() { return ""; },

  
  get SSLStatus() { return this._SSLStatus; },

  _update: function (state, status) {
      let deserialized = null;
      if (status) {
        let helper = Cc["@mozilla.org/network/serialization-helper;1"]
                      .getService(Components.interfaces.nsISerializationHelper);

        deserialized = helper.deserializeObject(status)
        deserialized.QueryInterface(Ci.nsISSLStatus);
      }

      
      
      if (deserialized && deserialized.isExtendedValidation)
        state |= Ci.nsIWebProgressListener.STATE_IDENTITY_EV_TOPLEVEL;

      this._state = state;
      this._SSLStatus = deserialized;
  }
};
