
















"use strict";




const { interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/systemlibs.js");

XPCOMUtils.defineLazyGetter(this, "SE", function() {
  let obj = {};
  Cu.import("resource://gre/modules/se_consts.js", obj);
  return obj;
});


let DEBUG = SE.DEBUG_CONNECTOR;
function debug(s) {
  if (DEBUG) {
    dump("-*- UiccConnector: " + s + "\n");
  }
}

XPCOMUtils.defineLazyModuleGetter(this, "SEUtils",
                                  "resource://gre/modules/SEUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "iccProvider",
                                   "@mozilla.org/ril/content-helper;1",
                                   "nsIIccProvider");

const UICCCONNECTOR_CONTRACTID =
  "@mozilla.org/secureelement/connector/uicc;1";
const UICCCONNECTOR_CID =
  Components.ID("{8e040e5d-c8c3-4c1b-ac82-c00d25d8c4a4}");
const NS_XPCOM_SHUTDOWN_OBSERVER_ID = "xpcom-shutdown";





const PREFERRED_UICC_CLIENTID =
  libcutils.property_get("ro.moz.se.def_client_id", "0");





function UiccConnector() {
  this._init();
}

UiccConnector.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISecureElementConnector]),
  classID: UICCCONNECTOR_CID,
  classInfo: XPCOMUtils.generateCI({
    classID: UICCCONNECTOR_CID,
    contractID: UICCCONNECTOR_CONTRACTID,
    classDescription: "UiccConnector",
    interfaces: [Ci.nsISecureElementConnector,
                 Ci.nsIIccListener,
                 Ci.nsIObserver]
  }),

  _isPresent: false,

  _init: function() {
    Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
    iccProvider.registerIccMsg(PREFERRED_UICC_CLIENTID, this);

    
    
    
    this._updatePresenceState();
  },

  _shutdown: function() {
    Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    iccProvider.unregisterIccMsg(PREFERRED_UICC_CLIENTID, this);
  },

  _updatePresenceState: function() {
    
    
    let notReadyStates = [
      "unknown",
      "illegal",
      "personalizationInProgress",
      "permanentBlocked",
    ];
    let cardState = iccProvider.getCardState(PREFERRED_UICC_CLIENTID);
    this._isPresent = cardState !== null &&
                      notReadyStates.indexOf(cardState) == -1;
  },

  
  _setChannelToCLAByte: function(cla, channel) {
    if (channel < SE.LOGICAL_CHANNEL_NUMBER_LIMIT) {
      
      cla = (cla & 0x9C) & 0xFF | channel;
    } else if (channel < SE.SUPPLEMENTARY_LOGICAL_CHANNEL_NUMBER_LIMIT) {
      
      cla = (cla & 0xB0) & 0xFF | 0x40 | (channel - SE.LOGICAL_CHANNEL_NUMBER_LIMIT);
    } else {
      debug("Channel number must be within [0..19]");
      return SE.ERROR_GENERIC;
    }
    return cla;
  },

  _doGetOpenResponse: function(channel, length, callback) {
    
    
    let cla = this._setChannelToCLAByte(SE.CLA_GET_RESPONSE, channel);
    this.exchangeAPDU(channel, cla, SE.INS_GET_RESPONSE, 0x00, 0x00,
                      null, length, {
      notifyExchangeAPDUResponse: function(sw1, sw2, response) {
        debug("GET Response : " + response);
        if (callback) {
          callback({
            error: SE.ERROR_NONE,
            sw1: sw1,
            sw2: sw2,
            response: response
          });
        }
      },

      notifyError: function(reason) {
        debug("Failed to get open response: " +
              ", Rejected with Reason : " + reason);
        if (callback) {
          callback({ error: SE.ERROR_INVALIDAPPLICATION, reason: reason });
        }
      }
    });
  },

  _doIccExchangeAPDU: function(channel, cla, ins, p1, p2, p3,
                               data, appendResp, callback) {
    iccProvider.iccExchangeAPDU(PREFERRED_UICC_CLIENTID, channel, cla & 0xFC,
                                ins, p1, p2, p3, data, {
      notifyExchangeAPDUResponse: (sw1, sw2, response) => {
        debug("sw1 : " + sw1 + ", sw2 : " + sw2 + ", response : " + response);

        
        
        
        
        
        
        if (sw1 === 0x6C) {
          
          

          
          
          this._doIccExchangeAPDU(channel, cla, ins, p1, p2,
                                  sw2, data, "", callback);
        } else if (sw1 === 0x61) {
          
          
          
          

          let claWithChannel = this._setChannelToCLAByte(SE.CLA_GET_RESPONSE,
                                                         channel);

          
          
          this._doIccExchangeAPDU(channel, claWithChannel, SE.INS_GET_RESPONSE,
                                  0x00, 0x00, sw2, null,
                                  (response ? response + appendResp : appendResp),
                                  callback);
        } else if (callback) {
          callback.notifyExchangeAPDUResponse(sw1, sw2, response);
        }
      },

      notifyError: (reason) => {
        debug("Failed to trasmit C-APDU over the channel #  : " + channel +
              ", Rejected with Reason : " + reason);
        if (callback) {
          callback.notifyError(reason);
        }
      }
    });
  },

  



  


  openChannel: function(aid, callback) {
    if (!this._isPresent) {
      callback.notifyError(SE.ERROR_NOTPRESENT);
      return;
    }

    
    
    
    
    
    
    iccProvider.iccOpenChannel(PREFERRED_UICC_CLIENTID, aid, {
      notifyOpenChannelSuccess: (channel) => {
        this._doGetOpenResponse(channel, 0x00, function(result) {
          if (callback) {
            callback.notifyOpenChannelSuccess(channel, result.response);
          }
        });
      },

      notifyError: (reason) => {
        debug("Failed to open the channel to AID : " + aid +
              ", Rejected with Reason : " + reason);
        if (callback) {
          callback.notifyError(reason);
        }
      }
    });
  },

  


  exchangeAPDU: function(channel, cla, ins, p1, p2, data, le, callback) {
    if (!this._isPresent) {
      callback.notifyError(SE.ERROR_NOTPRESENT);
      return;
    }

    if (data && data.length % 2 !== 0) {
      callback.notifyError("Data should be a hex string with length % 2 === 0");
      return;
    }

    cla = this._setChannelToCLAByte(cla, channel);
    let lc = data ? data.length / 2 : 0;
    let p3 = lc || le;

    if (lc && (le !== -1)) {
      data += SEUtils.byteArrayToHexString([le]);
    }

    
    
    debug("exchangeAPDU on Channel # " + channel);
    this._doIccExchangeAPDU(channel, cla, ins, p1, p2, p3, data, "",
                            callback);
  },

  


  closeChannel: function(channel, callback) {
    if (!this._isPresent) {
      callback.notifyError(SE.ERROR_NOTPRESENT);
      return;
    }

    iccProvider.iccCloseChannel(PREFERRED_UICC_CLIENTID, channel, {
      notifyCloseChannelSuccess: function() {
        debug("closeChannel successfully closed the channel # : " + channel);
        if (callback) {
          callback.notifyCloseChannelSuccess();
        }
      },

      notifyError: function(reason) {
        debug("Failed to close the channel #  : " + channel +
              ", Rejected with Reason : " + reason);
        if (callback) {
          callback.notifyError(reason);
        }
      }
    });
  },

  


  notifyStkCommand: function() {},

  notifyStkSessionEnd: function() {},

  notifyIccInfoChanged: function() {},

  notifyCardStateChanged: function() {
    this._updatePresenceState();
  },

  



  observe: function(subject, topic, data) {
    if (topic === NS_XPCOM_SHUTDOWN_OBSERVER_ID) {
      this._shutdown();
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([UiccConnector]);
