




"use strict";







(function(exports) {

  
  let DEBUG = DEBUG_WORKER;
  
  let Buf = require("resource://gre/modules/workers/worker_buf.js").Buf;

  let BufObject = function(aContext) {
    this.context = aContext;
    
    this.mToken = 1;
    
    
    this.mTokenRequestMap = new Map();
    
    
    
    Buf._init.apply(this);
  };

  


  BufObject.prototype = Object.create(Buf);

  


  BufObject.prototype.processParcel = function() {
    let responseType = this.readInt32();

    let requestType, options;
    if (responseType == RESPONSE_TYPE_SOLICITED) {
      let token = this.readInt32();
      let error = this.readInt32();

      options = this.mTokenRequestMap.get(token);
      if (!options) {
        if (DEBUG) {
          this.context.debug("Suspicious uninvited request found: " +
                             token + ". Ignored!");
        }
        return;
      }

      this.mTokenRequestMap.delete(token);
      requestType = options.rilRequestType;

      options.rilRequestError = error;
      if (DEBUG) {
        this.context.debug("Solicited response for request type " + requestType +
                           ", token " + token + ", error " + error);
      }
    } else if (responseType == RESPONSE_TYPE_UNSOLICITED) {
      requestType = this.readInt32();
      if (DEBUG) {
        this.context.debug("Unsolicited response for request type " + requestType);
      }
    } else {
      if (DEBUG) {
        this.context.debug("Unknown response type: " + responseType);
      }
      return;
    }

    this.context.RIL.handleParcel(requestType, this.readAvailable, options);
  };

  








  BufObject.prototype.newParcel = function(type, options) {
    if (DEBUG) {
      this.context.debug("New outgoing parcel of type " + type);
    }

    
    this.outgoingIndex = this.PARCEL_SIZE_SIZE;
    this.writeInt32(this._reMapRequestType(type));
    this.writeInt32(this.mToken);

    if (!options) {
      options = {};
    }
    options.rilRequestType = type;
    options.rilRequestError = null;
    this.mTokenRequestMap.set(this.mToken, options);
    this.mToken++;
    return this.mToken;
  };

  BufObject.prototype.simpleRequest = function(type, options) {
    this.newParcel(type, options);
    this.sendParcel();
  };

  BufObject.prototype.onSendParcel = function(parcel) {
    self.postRILMessage(this.context.clientId, parcel);
  };

  





  BufObject.prototype._reMapRequestType = function(type) {
    let newType = type;
    switch (type) {
      case REQUEST_SET_UICC_SUBSCRIPTION:
      case REQUEST_SET_DATA_SUBSCRIPTION:
        if (this.context.RIL.version < 9) {
          
          
          newType = type - 1;
        }
        break;
    }

    return newType;
  };

  
  
  
  exports.BufObject = BufObject;
})(self); 

