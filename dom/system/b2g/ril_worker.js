





























































"use strict";

importScripts("ril_consts.js", "systemlibs.js");




let DEBUG;

const INT32_MAX   = 2147483647;
const UINT8_SIZE  = 1;
const UINT16_SIZE = 2;
const UINT32_SIZE = 4;
const PARCEL_SIZE_SIZE = UINT32_SIZE;

let RILQUIRKS_CALLSTATE_EXTRA_UINT32 = false;













let Buf = {

  INCOMING_BUFFER_LENGTH: 1024,
  OUTGOING_BUFFER_LENGTH: 1024,

  init: function init() {
    this.incomingBuffer = new ArrayBuffer(this.INCOMING_BUFFER_LENGTH);
    this.outgoingBuffer = new ArrayBuffer(this.OUTGOING_BUFFER_LENGTH);

    this.incomingBytes = new Uint8Array(this.incomingBuffer);
    this.outgoingBytes = new Uint8Array(this.outgoingBuffer);

    
    this.incomingWriteIndex = 0;
    this.incomingReadIndex = 0;

    
    this.outgoingIndex = PARCEL_SIZE_SIZE;

    
    this.readIncoming = 0;

    
    this.readAvailable = 0;

    
    
    this.currentParcelSize = 0;

    
    this.token = 1;

    
    
    this.tokenRequestMap = {};

    
    this.lastSolicitedToken = 0;
  },

  






  growIncomingBuffer: function growIncomingBuffer(min_size) {
    if (DEBUG) {
      debug("Current buffer of " + this.INCOMING_BUFFER_LENGTH +
            " can't handle incoming " + min_size + " bytes.");
    }
    let oldBytes = this.incomingBytes;
    this.INCOMING_BUFFER_LENGTH =
      2 << Math.floor(Math.log(min_size)/Math.log(2));
    if (DEBUG) debug("New incoming buffer size: " + this.INCOMING_BUFFER_LENGTH);
    this.incomingBuffer = new ArrayBuffer(this.INCOMING_BUFFER_LENGTH);
    this.incomingBytes = new Uint8Array(this.incomingBuffer);
    if (this.incomingReadIndex <= this.incomingWriteIndex) {
      
      
      
      this.incomingBytes.set(oldBytes, 0);
    } else {
      
      
      
      
      let head = oldBytes.subarray(this.incomingReadIndex);
      let tail = oldBytes.subarray(0, this.incomingReadIndex);
      this.incomingBytes.set(head, 0);
      this.incomingBytes.set(tail, head.length);
      this.incomingReadIndex = 0;
      this.incomingWriteIndex += head.length;
    }
    if (DEBUG) {
      debug("New incoming buffer size is " + this.INCOMING_BUFFER_LENGTH);
    }
  },

  






  growOutgoingBuffer: function growOutgoingBuffer(min_size) {
    if (DEBUG) {
      debug("Current buffer of " + this.OUTGOING_BUFFER_LENGTH +
            " is too small.");
    }
    let oldBytes = this.outgoingBytes;
    this.OUTGOING_BUFFER_LENGTH =
      2 << Math.floor(Math.log(min_size)/Math.log(2));
    this.outgoingBuffer = new ArrayBuffer(this.OUTGOING_BUFFER_LENGTH);
    this.outgoingBytes = new Uint8Array(this.outgoingBuffer);
    this.outgoingBytes.set(oldBytes, 0);
    if (DEBUG) {
      debug("New outgoing buffer size is " + this.OUTGOING_BUFFER_LENGTH);
    }
  },

  





  readUint8Unchecked: function readUint8Unchecked() {
    let value = this.incomingBytes[this.incomingReadIndex];
    this.incomingReadIndex = (this.incomingReadIndex + 1) %
                             this.INCOMING_BUFFER_LENGTH;
    return value;
  },

  readUint8: function readUint8() {
    if (!this.readAvailable) {
      throw new Error("Trying to read data beyond the parcel end!");
    }

    this.readAvailable--;
    return this.readUint8Unchecked();
  },

  readUint16: function readUint16() {
    return this.readUint8() | this.readUint8() << 8;
  },

  readUint32: function readUint32() {
    return this.readUint8()       | this.readUint8() <<  8 |
           this.readUint8() << 16 | this.readUint8() << 24;
  },

  readUint32List: function readUint32List() {
    let length = this.readUint32();
    let ints = [];
    for (let i = 0; i < length; i++) {
      ints.push(this.readUint32());
    }
    return ints;
  },

  readString: function readString() {
    let string_len = this.readUint32();
    if (string_len < 0 || string_len >= INT32_MAX) {
      return null;
    }
    let s = "";
    for (let i = 0; i < string_len; i++) {
      s += String.fromCharCode(this.readUint16());
    }
    
    
    
    let delimiter = this.readUint16();
    if (!(string_len & 1)) {
      delimiter |= this.readUint16();
    }
    if (DEBUG) {
      if (delimiter != 0) {
        debug("Something's wrong, found string delimiter: " + delimiter);
      }
    }
    return s;
  },

  readStringList: function readStringList() {
    let num_strings = this.readUint32();
    let strings = [];
    for (let i = 0; i < num_strings; i++) {
      strings.push(this.readString());
    }
    return strings;
  },

  readParcelSize: function readParcelSize() {
    return this.readUint8Unchecked() << 24 |
           this.readUint8Unchecked() << 16 |
           this.readUint8Unchecked() <<  8 |
           this.readUint8Unchecked();
  },

  



  writeUint8: function writeUint8(value) {
    if (this.outgoingIndex >= this.OUTGOING_BUFFER_LENGTH) {
      this.growOutgoingBuffer(this.outgoingIndex + 1);
    }
    this.outgoingBytes[this.outgoingIndex] = value;
    this.outgoingIndex++;
  },

  writeUint16: function writeUint16(value) {
    this.writeUint8(value & 0xff);
    this.writeUint8((value >> 8) & 0xff);
  },

  writeUint32: function writeUint32(value) {
    this.writeUint8(value & 0xff);
    this.writeUint8((value >> 8) & 0xff);
    this.writeUint8((value >> 16) & 0xff);
    this.writeUint8((value >> 24) & 0xff);
  },

  writeString: function writeString(value) {
    if (value == null) {
      this.writeUint32(-1);
      return;
    }
    this.writeUint32(value.length);
    for (let i = 0; i < value.length; i++) {
      this.writeUint16(value.charCodeAt(i));
    }
    
    
    
    this.writeUint16(0);
    if (!(value.length & 1)) {
      this.writeUint16(0);
    }
  },

  writeStringList: function writeStringList(strings) {
    this.writeUint32(strings.length);
    for (let i = 0; i < strings.length; i++) {
      this.writeString(strings[i]);
    }
  },

  writeParcelSize: function writeParcelSize(value) {
    




    let currentIndex = this.outgoingIndex;
    this.outgoingIndex = 0;
    this.writeUint8((value >> 24) & 0xff);
    this.writeUint8((value >> 16) & 0xff);
    this.writeUint8((value >> 8) & 0xff);
    this.writeUint8(value & 0xff);
    this.outgoingIndex = currentIndex;
  },


  



  





  writeToIncoming: function writeToIncoming(incoming) {
    
    
    
    
    if (incoming.length > this.INCOMING_BUFFER_LENGTH) {
      this.growIncomingBuffer(incoming.length);
    }

    
    
    let remaining = this.INCOMING_BUFFER_LENGTH - this.incomingWriteIndex;
    if (remaining >= incoming.length) {
      this.incomingBytes.set(incoming, this.incomingWriteIndex);
    } else {
      
      let head = incoming.subarray(0, remaining);
      let tail = incoming.subarray(remaining);
      this.incomingBytes.set(head, this.incomingWriteIndex);
      this.incomingBytes.set(tail, 0);
    }
    this.incomingWriteIndex = (this.incomingWriteIndex + incoming.length) %
                              this.INCOMING_BUFFER_LENGTH;
  },

  





  processIncoming: function processIncoming(incoming) {
    if (DEBUG) {
      debug("Received " + incoming.length + " bytes.");
      debug("Already read " + this.readIncoming);
    }

    this.writeToIncoming(incoming);
    this.readIncoming += incoming.length;
    while (true) {
      if (!this.currentParcelSize) {
        
        if (this.readIncoming < PARCEL_SIZE_SIZE) {
          
          
          if (DEBUG) debug("Next parcel size unknown, going to sleep.");
          return;
        }
        this.currentParcelSize = this.readParcelSize();
        if (DEBUG) debug("New incoming parcel of size " +
                         this.currentParcelSize);
        
        this.readIncoming -= PARCEL_SIZE_SIZE;
      }

      if (this.readIncoming < this.currentParcelSize) {
        
        if (DEBUG) debug("Read " + this.readIncoming + ", but parcel size is "
                         + this.currentParcelSize + ". Going to sleep.");
        return;
      }

      
      
      let expectedAfterIndex = (this.incomingReadIndex + this.currentParcelSize)
                               % this.INCOMING_BUFFER_LENGTH;

      if (DEBUG) {
        let parcel;
        if (expectedAfterIndex < this.incomingReadIndex) {
          let head = this.incomingBytes.subarray(this.incomingReadIndex);
          let tail = this.incomingBytes.subarray(0, expectedAfterIndex);
          parcel = Array.slice(head).concat(Array.slice(tail));
        } else {
          parcel = Array.slice(this.incomingBytes.subarray(
            this.incomingReadIndex, expectedAfterIndex));
        }
        debug("Parcel (size " + this.currentParcelSize + "): " + parcel);
      }

      if (DEBUG) debug("We have at least one complete parcel.");
      try {
        this.readAvailable = this.currentParcelSize;
        this.processParcel();
      } catch (ex) {
        if (DEBUG) debug("Parcel handling threw " + ex + "\n" + ex.stack);
      }

      
      if (this.incomingReadIndex != expectedAfterIndex) {
        if (DEBUG) {
          debug("Parcel handler didn't consume whole parcel, " +
                Math.abs(expectedAfterIndex - this.incomingReadIndex) +
                " bytes left over");
        }
        this.incomingReadIndex = expectedAfterIndex;
      }
      this.readIncoming -= this.currentParcelSize;
      this.readAvailable = 0;
      this.currentParcelSize = 0;
    }
  },

  


  processParcel: function processParcel() {
    let response_type = this.readUint32();

    let request_type, options;
    if (response_type == RESPONSE_TYPE_SOLICITED) {
      let token = this.readUint32();
      let error = this.readUint32();

      options = this.tokenRequestMap[token];
      request_type = options.rilRequestType;
      if (error) {
        
        if (DEBUG) {
          debug("Received error " + error + " for solicited parcel type " +
                request_type);
        }
        return;
      }
      if (DEBUG) {
        debug("Solicited response for request type " + request_type +
              ", token " + token);
      }
      delete this.tokenRequestMap[token];
      this.lastSolicitedToken = token;
    } else if (response_type == RESPONSE_TYPE_UNSOLICITED) {
      request_type = this.readUint32();
      if (DEBUG) debug("Unsolicited response for request type " + request_type);
    } else {
      if (DEBUG) debug("Unknown response type: " + response_type);
      return;
    }

    RIL.handleParcel(request_type, this.readAvailable, options);
  },

  








  newParcel: function newParcel(type, options) {
    if (DEBUG) debug("New outgoing parcel of type " + type);
    
    this.outgoingIndex = PARCEL_SIZE_SIZE;
    this.writeUint32(type);
    let token = this.token;
    this.writeUint32(token);

    if (!options) {
      options = {};
    }
    options.rilRequestType = type;
    this.tokenRequestMap[token] = options;
    this.token++;
    return token;
  },

  


  sendParcel: function sendParcel() {
    
    
    
    let parcelSize = this.outgoingIndex - PARCEL_SIZE_SIZE;
    this.writeParcelSize(parcelSize);

    
    
    let parcel = this.outgoingBytes.subarray(0, this.outgoingIndex);
    if (DEBUG) debug("Outgoing parcel: " + Array.slice(parcel));
    postRILMessage(parcel);
    this.outgoingIndex = PARCEL_SIZE_SIZE;
  },

  simpleRequest: function simpleRequest(type) {
    this.newParcel(type);
    this.sendParcel();
  }
};








let RIL = {

  




  rilQuirksInitialized: false,
  initRILQuirks: function initRILQuirks() {
    
    
    let model_id = libcutils.property_get("ril.model_id");
    if (DEBUG) debug("Detected RIL model " + model_id);
    if (model_id == "I9100") {
      if (DEBUG) debug("Enabling RILQUIRKS_CALLSTATE_EXTRA_UINT32 for I9100.");
      RILQUIRKS_CALLSTATE_EXTRA_UINT32 = true;
    }

    this.rilQuirksInitialized = true;
  },

  









  parseInt: function RIL_parseInt(string, defaultValue) {
    let number = parseInt(string, 10);
    if (!isNaN(number)) {
      return number;
    }
    if (defaultValue === undefined) {
      defaultValue = null;
    }
    return defaultValue;
  },

  




  getICCStatus: function getICCStatus() {
    Buf.simpleRequest(REQUEST_GET_SIM_STATUS);
  },

  







  enterICCPIN: function enterICCPIN(pin) {
    Buf.newParcel(REQUEST_ENTER_SIM_PIN);
    Buf.writeUint32(1);
    Buf.writeString(pin);
    Buf.sendParcel();
  },

  









  changeICCPIN: function changeICCPIN(oldPin, newPin) {
    Buf.newParcel(REQUEST_CHANGE_SIM_PIN);
    Buf.writeUint32(2);
    Buf.writeString(oldPin);
    Buf.writeString(newPin);
    Buf.sendParcel();
  },

  









   enterICCPUK: function enterICCPUK(puk, newPin) {
     Buf.newParcel(REQUEST_ENTER_SIM_PUK);
     Buf.writeUint32(2);
     Buf.writeString(puk);
     Buf.writeString(newPin);
     Buf.sendParcel();
   },

  





  setRadioPower: function setRadioPower(on) {
    Buf.newParcel(REQUEST_RADIO_POWER);
    Buf.writeUint32(1);
    Buf.writeUint32(on ? 1 : 0);
    Buf.sendParcel();
  },

  





  setScreenState: function setScreenState(on) {
    Buf.newParcel(REQUEST_SCREEN_STATE);
    Buf.writeUint32(1);
    Buf.writeUint32(on ? 1 : 0);
    Buf.sendParcel();
  },

  getRegistrationState: function getRegistrationState() {
    Buf.simpleRequest(REQUEST_REGISTRATION_STATE);
  },

  getGPRSRegistrationState: function getGPRSRegistrationState() {
    Buf.simpleRequest(REQUEST_GPRS_REGISTRATION_STATE);
  },

  getOperator: function getOperator() {
    Buf.simpleRequest(REQUEST_OPERATOR);
  },

  getNetworkSelectionMode: function getNetworkSelectionMode() {
    Buf.simpleRequest(REQUEST_QUERY_NETWORK_SELECTION_MODE);
  },

  


  getCurrentCalls: function getCurrentCalls() {
    Buf.simpleRequest(REQUEST_GET_CURRENT_CALLS);
  },

  


  getSignalStrength: function getSignalStrength() {
    Buf.simpleRequest(REQUEST_SIGNAL_STRENGTH);
  },

  getIMEI: function getIMEI() {
    Buf.simpleRequest(REQUEST_GET_IMEI);
  },

  getIMEISV: function getIMEISV() {
    Buf.simpleRequest(REQUEST_GET_IMEISV);
  },

  getDeviceIdentity: function getDeviceIdentity() {
    Buf.simpleRequest(REQUEST_GET_DEVICE_IDENTITY);
  },

  









  dial: function dial(address, clirMode, uusInfo) {
    let token = Buf.newParcel(REQUEST_DIAL);
    Buf.writeString(address);
    Buf.writeUint32(clirMode || 0);
    Buf.writeUint32(uusInfo || 0);
    
    
    Buf.writeUint32(0);
    Buf.sendParcel();
  },

  





  hangUp: function hangUp(callIndex) {
    Buf.newParcel(REQUEST_HANGUP);
    Buf.writeUint32(1);
    Buf.writeUint32(callIndex);
    Buf.sendParcel();
  },

  





  setMute: function setMute(mute) {
    Buf.newParcel(REQUEST_SET_MUTE);
    Buf.writeUint32(1);
    Buf.writeUint32(mute ? 1 : 0);
    Buf.sendParcel();
  },

  


  answerCall: function answerCall() {
    Buf.simpleRequest(REQUEST_ANSWER);
  },

  


  rejectCall: function rejectCall() {
    Buf.simpleRequest(REQUEST_UDUB);
  },

  
















  sendSMS: function sendSMS(options) {
    let token = Buf.newParcel(REQUEST_SEND_SMS, options);
    
    
    
    
    Buf.writeUint32(2);
    Buf.writeString(options.SMSC);
    GsmPDUHelper.writeMessage(options.number,
                              options.body,
                              options.dcs,
                              options.bodyLengthInOctets);
    Buf.sendParcel();
  },

  







  acknowledgeSMS: function acknowledgeSMS(success, cause) {
    let token = Buf.newParcel(REQUEST_SMS_ACKNOWLEDGE);
    Buf.writeUint32(2);
    Buf.writeUint32(success ? 1 : 0);
    Buf.writeUint32(cause);
    Buf.sendParcel();
  },

  





  startTone: function startTone(dtmfChar) {
    Buf.newParcel(REQUEST_DTMF_START);
    Buf.writeString(dtmfChar);
    Buf.sendParcel();
  },

  stopTone: function stopTone() {
    Buf.simpleRequest(REQUEST_DTMF_STOP);
  },

  sendTone: function sendTone(dtmfChar) {
    Buf.newParcel(REQUEST_DTMF);
    Buf.writeString(dtmfChar);
    Buf.sendParcel();
  },

  


  getSMSCAddress: function getSMSCAddress() {
    Buf.simpleRequest(REQUEST_GET_SMSC_ADDRESS);
  },

  





  setSMSCAddress: function setSMSCAddress(smsc) {
    Buf.newParcel(REQUEST_SET_SMSC_ADDRESS);
    Buf.writeString(smsc);
    Buf.sendParcel();
  },

  





















  setupDataCall: function (radioTech, apn, user, passwd, chappap, pdptype) {
    let token = Buf.newParcel(REQUEST_SETUP_DATA_CALL);
    Buf.writeUint32(7);
    Buf.writeString(radioTech.toString());
    Buf.writeString(DATACALL_PROFILE_DEFAULT.toString());
    Buf.writeString(apn);
    Buf.writeString(user);
    Buf.writeString(passwd);
    Buf.writeString(chappap.toString());
    Buf.writeString(pdptype);
    Buf.sendParcel();
    return token;
  },

  







  deactivateDataCall: function (cid, reason) {
    let token = Buf.newParcel(REQUEST_DEACTIVATE_DATA_CALL);
    Buf.writeUint32(2);
    Buf.writeString(cid);
    Buf.writeString(reason);
    Buf.sendParcel();
    return token;
  },

  


  getDataCallList: function getDataCallList() {
    Buf.simpleRequest(REQUEST_DATA_CALL_LIST);
  },

  


  getFailCauseCode: function getFailCauseCode() {
    Buf.simpleRequest(REQUEST_LAST_CALL_FAIL_CAUSE);
  },

  





  handleParcel: function handleParcel(request_type, length, options) {
    let method = this[request_type];
    if (typeof method == "function") {
      if (DEBUG) debug("Handling parcel as " + method.name);
      method.call(this, length, options);
    }
  }
};

RIL[REQUEST_GET_SIM_STATUS] = function REQUEST_GET_SIM_STATUS() {
  let iccStatus = {
    cardState:                   Buf.readUint32(), 
    universalPINState:           Buf.readUint32(), 
    gsmUmtsSubscriptionAppIndex: Buf.readUint32(),
    setCdmaSubscriptionAppIndex: Buf.readUint32(),
    apps:                        []
  };

  let apps_length = Buf.readUint32();
  if (apps_length > CARD_MAX_APPS) {
    apps_length = CARD_MAX_APPS;
  }

  for (let i = 0 ; i < apps_length ; i++) {
    iccStatus.apps.push({
      app_type:       Buf.readUint32(), 
      app_state:      Buf.readUint32(), 
      perso_substate: Buf.readUint32(), 
      aid:            Buf.readString(),
      app_label:      Buf.readString(),
      pin1_replaced:  Buf.readUint32(),
      pin1:           Buf.readUint32(),
      pin2:           Buf.readUint32()
    });
  }
  Phone.onICCStatus(iccStatus);
};
RIL[REQUEST_ENTER_SIM_PIN] = function REQUEST_ENTER_SIM_PIN() {
  let response = Buf.readUint32List();
  Phone.onEnterICCPIN(response);
};
RIL[REQUEST_ENTER_SIM_PUK] = function REQUEST_ENTER_SIM_PUK() {
  let response = Buf.readUint32List();
  Phone.onEnterICCPUK(response);
};
RIL[REQUEST_ENTER_SIM_PIN2] = null;
RIL[REQUEST_ENTER_SIM_PUK2] = null;
RIL[REQUEST_CHANGE_SIM_PIN] = function REQUEST_CHANGE_SIM_PIN() {
  Phone.onChangeICCPIN();
};
RIL[REQUEST_CHANGE_SIM_PIN2] = null;
RIL[REQUEST_ENTER_NETWORK_DEPERSONALIZATION] = null;
RIL[REQUEST_GET_CURRENT_CALLS] = function REQUEST_GET_CURRENT_CALLS(length) {
  if (!this.rilQuirksInitialized) {
    this.initRILQuirks();
  }

  let calls_length = 0;
  
  
  if (length) {
    calls_length = Buf.readUint32();
  }
  if (!calls_length) {
    Phone.onCurrentCalls(null);
    return;
  }

  let calls = {};
  for (let i = 0; i < calls_length; i++) {
    let call = {};
    call.state          = Buf.readUint32(); 
    call.callIndex      = Buf.readUint32(); 
    call.toa            = Buf.readUint32();
    call.isMpty         = Boolean(Buf.readUint32());
    call.isMT           = Boolean(Buf.readUint32());
    call.als            = Buf.readUint32();
    call.isVoice        = Boolean(Buf.readUint32());
    call.isVoicePrivacy = Boolean(Buf.readUint32());
    if (RILQUIRKS_CALLSTATE_EXTRA_UINT32) {
      Buf.readUint32();
    }
    call.number             = Buf.readString(); 
    call.numberPresentation = Buf.readUint32(); 
    call.name               = Buf.readString();
    call.namePresentation   = Buf.readUint32();

    call.uusInfo = null;
    let uusInfoPresent = Buf.readUint32();
    if (uusInfoPresent == 1) {
      call.uusInfo = {
        type:     Buf.readUint32(),
        dcs:      Buf.readUint32(),
        userData: null 
      };
    }

    calls[call.callIndex] = call;
  }
  Phone.onCurrentCalls(calls);
};
RIL[REQUEST_DIAL] = function REQUEST_DIAL(length) {
  Phone.onDial();
};
RIL[REQUEST_GET_IMSI] = function REQUEST_GET_IMSI(length) {
  let imsi = Buf.readString();
  Phone.onIMSI(imsi);
};
RIL[REQUEST_HANGUP] = function REQUEST_HANGUP(length) {
  Phone.onHangUp();
};
RIL[REQUEST_HANGUP_WAITING_OR_BACKGROUND] = null;
RIL[REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND] = null;
RIL[REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE] = null;
RIL[REQUEST_SWITCH_HOLDING_AND_ACTIVE] = null;
RIL[REQUEST_CONFERENCE] = null;
RIL[REQUEST_UDUB] = function REQUEST_UDUB(length) {
  Phone.onRejectCall();
};
RIL[REQUEST_LAST_CALL_FAIL_CAUSE] = null;
RIL[REQUEST_SIGNAL_STRENGTH] = function REQUEST_SIGNAL_STRENGTH() {
  let strength = {
    
    
    
    gsmSignalStrength: Buf.readUint32() & 0xff,
    
    gsmBitErrorRate:   Buf.readUint32(),
    
    cdmaDBM:           Buf.readUint32(),
    
    cdmaECIO:          Buf.readUint32(),
    
    evdoDBM:           Buf.readUint32(),
    
    evdoECIO:          Buf.readUint32(),
    
    evdoSNR:           Buf.readUint32()
  };
  Phone.onSignalStrength(strength);
};
RIL[REQUEST_REGISTRATION_STATE] = function REQUEST_REGISTRATION_STATE(length) {
  let state = Buf.readStringList();
  Phone.onRegistrationState(state);
};
RIL[REQUEST_GPRS_REGISTRATION_STATE] = function REQUEST_GPRS_REGISTRATION_STATE(length) {
  let state = Buf.readStringList();
  Phone.onGPRSRegistrationState(state);
};
RIL[REQUEST_OPERATOR] = function REQUEST_OPERATOR(length) {
  let operator = Buf.readStringList();
  Phone.onOperator(operator);
};
RIL[REQUEST_RADIO_POWER] = null;
RIL[REQUEST_DTMF] = function REQUEST_DTMF() {
  Phone.onSendTone();
};
RIL[REQUEST_SEND_SMS] = function REQUEST_SEND_SMS(length, options) {
  options.messageRef = Buf.readUint32();
  options.ackPDU = Buf.readString();
  options.errorCode = Buf.readUint32();
  Phone.onSendSMS(options);
};
RIL[REQUEST_SEND_SMS_EXPECT_MORE] = null;
RIL[REQUEST_SETUP_DATA_CALL] = function REQUEST_SETUP_DATA_CALL() {
  let [cid, ifname, ipaddr, dns, gw] = Buf.readStringList();
  Phone.onSetupDataCall(Buf.lastSolicitedToken, cid, ifname, ipaddr, dns, gw);
};
RIL[REQUEST_SIM_IO] = null;
RIL[REQUEST_SEND_USSD] = null;
RIL[REQUEST_CANCEL_USSD] = null;
RIL[REQUEST_GET_CLIR] = null;
RIL[REQUEST_SET_CLIR] = null;
RIL[REQUEST_QUERY_CALL_FORWARD_STATUS] = null;
RIL[REQUEST_SET_CALL_FORWARD] = null;
RIL[REQUEST_QUERY_CALL_WAITING] = null;
RIL[REQUEST_SET_CALL_WAITING] = null;
RIL[REQUEST_SMS_ACKNOWLEDGE] = function REQUEST_SMS_ACKNOWLEDGE() {
  Phone.onAcknowledgeSMS();
};
RIL[REQUEST_GET_IMEI] = function REQUEST_GET_IMEI() {
  let imei = Buf.readString();
  Phone.onIMEI(imei);
};
RIL[REQUEST_GET_IMEISV] = function REQUEST_GET_IMEISV() {
  let imeiSV = Buf.readString();
  Phone.onIMEISV(imeiSV);
};
RIL[REQUEST_ANSWER] = function REQUEST_ANSWER(length) {
  Phone.onAnswerCall();
};
RIL[REQUEST_DEACTIVATE_DATA_CALL] = function REQUEST_DEACTIVATE_DATA_CALL() {
  Phone.onDeactivateDataCall(Buf.lastSolicitedToken);
};
RIL[REQUEST_QUERY_FACILITY_LOCK] = null;
RIL[REQUEST_SET_FACILITY_LOCK] = null;
RIL[REQUEST_CHANGE_BARRING_PASSWORD] = null;
RIL[REQUEST_QUERY_NETWORK_SELECTION_MODE] = function REQUEST_QUERY_NETWORK_SELECTION_MODE() {
  let response = Buf.readUint32List();
  Phone.onNetworkSelectionMode(response);
};
RIL[REQUEST_SET_NETWORK_SELECTION_AUTOMATIC] = null;
RIL[REQUEST_SET_NETWORK_SELECTION_MANUAL] = null;
RIL[REQUEST_QUERY_AVAILABLE_NETWORKS] = null;
RIL[REQUEST_DTMF_START] = function REQUEST_DTMF_START() {
  Phone.onStartTone();
};
RIL[REQUEST_DTMF_STOP] = function REQUEST_DTMF_STOP() {
  Phone.onStopTone();
};
RIL[REQUEST_BASEBAND_VERSION] = function REQUEST_BASEBAND_VERSION() {
  let version = Buf.readString();
  Phone.onBasebandVersion(version);
};
RIL[REQUEST_SEPARATE_CONNECTION] = null;
RIL[REQUEST_SET_MUTE] = function REQUEST_SET_MUTE(length) {
  Phone.onSetMute();
};
RIL[REQUEST_GET_MUTE] = null;
RIL[REQUEST_QUERY_CLIP] = null;
RIL[REQUEST_LAST_DATA_CALL_FAIL_CAUSE] = null;
RIL[REQUEST_DATA_CALL_LIST] = function REQUEST_DATA_CALL_LIST(length) {
  let num = 0;
  if (length) {
    num = Buf.readUint32();
  }
  if (!num) {
    Phone.onDataCallList(null);
    return;
  }

  let datacalls = {};
  for (let i = 0; i < num; i++) {
    let datacall = {
      cid: Buf.readUint32().toString(),
      active: Buf.readUint32(),
      type: Buf.readString(),
      apn: Buf.readString(),
      address: Buf.readString()
    };
    datacalls[datacall.cid] = datacall;
  }

  Phone.onDataCallList(datacalls);
};
RIL[REQUEST_RESET_RADIO] = null;
RIL[REQUEST_OEM_HOOK_RAW] = null;
RIL[REQUEST_OEM_HOOK_STRINGS] = null;
RIL[REQUEST_SCREEN_STATE] = null;
RIL[REQUEST_SET_SUPP_SVC_NOTIFICATION] = null;
RIL[REQUEST_WRITE_SMS_TO_SIM] = null;
RIL[REQUEST_DELETE_SMS_ON_SIM] = null;
RIL[REQUEST_SET_BAND_MODE] = null;
RIL[REQUEST_QUERY_AVAILABLE_BAND_MODE] = null;
RIL[REQUEST_STK_GET_PROFILE] = null;
RIL[REQUEST_STK_SET_PROFILE] = null;
RIL[REQUEST_STK_SEND_ENVELOPE_COMMAND] = null;
RIL[REQUEST_STK_SEND_TERMINAL_RESPONSE] = null;
RIL[REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM] = null;
RIL[REQUEST_EXPLICIT_CALL_TRANSFER] = null;
RIL[REQUEST_SET_PREFERRED_NETWORK_TYPE] = null;
RIL[REQUEST_GET_PREFERRED_NETWORK_TYPE] = null;
RIL[REQUEST_GET_NEIGHBORING_CELL_IDS] = null;
RIL[REQUEST_SET_LOCATION_UPDATES] = null;
RIL[REQUEST_CDMA_SET_SUBSCRIPTION] = null;
RIL[REQUEST_CDMA_SET_ROAMING_PREFERENCE] = null;
RIL[REQUEST_CDMA_QUERY_ROAMING_PREFERENCE] = null;
RIL[REQUEST_SET_TTY_MODE] = null;
RIL[REQUEST_QUERY_TTY_MODE] = null;
RIL[REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE] = null;
RIL[REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE] = null;
RIL[REQUEST_CDMA_FLASH] = null;
RIL[REQUEST_CDMA_BURST_DTMF] = null;
RIL[REQUEST_CDMA_VALIDATE_AND_WRITE_AKEY] = null;
RIL[REQUEST_CDMA_SEND_SMS] = null;
RIL[REQUEST_CDMA_SMS_ACKNOWLEDGE] = null;
RIL[REQUEST_GSM_GET_BROADCAST_SMS_CONFIG] = null;
RIL[REQUEST_GSM_SET_BROADCAST_SMS_CONFIG] = null;
RIL[REQUEST_GSM_SMS_BROADCAST_ACTIVATION] = null;
RIL[REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG] = null;
RIL[REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG] = null;
RIL[REQUEST_CDMA_SMS_BROADCAST_ACTIVATION] = null;
RIL[REQUEST_CDMA_SUBSCRIPTION] = null;
RIL[REQUEST_CDMA_WRITE_SMS_TO_RUIM] = null;
RIL[REQUEST_CDMA_DELETE_SMS_ON_RUIM] = null;
RIL[REQUEST_DEVICE_IDENTITY] = null;
RIL[REQUEST_EXIT_EMERGENCY_CALLBACK_MODE] = null;
RIL[REQUEST_GET_SMSC_ADDRESS] = function REQUEST_GET_SMSC_ADDRESS() {
  let smsc = Buf.readString();
  Phone.onGetSMSCAddress(smsc);
};
RIL[REQUEST_SET_SMSC_ADDRESS] = function REQUEST_SET_SMSC_ADDRESS() {
  Phone.onSetSMSCAddress();
};
RIL[REQUEST_REPORT_SMS_MEMORY_STATUS] = null;
RIL[REQUEST_REPORT_STK_SERVICE_IS_RUNNING] = null;
RIL[UNSOLICITED_RESPONSE_RADIO_STATE_CHANGED] = function UNSOLICITED_RESPONSE_RADIO_STATE_CHANGED() {
  let newState = Buf.readUint32();
  Phone.onRadioStateChanged(newState);
};
RIL[UNSOLICITED_RESPONSE_CALL_STATE_CHANGED] = function UNSOLICITED_RESPONSE_CALL_STATE_CHANGED() {
  Phone.onCallStateChanged();
};
RIL[UNSOLICITED_RESPONSE_NETWORK_STATE_CHANGED] = function UNSOLICITED_RESPONSE_NETWORK_STATE_CHANGED() {
  Phone.onNetworkStateChanged();
};
RIL[UNSOLICITED_RESPONSE_NEW_SMS] = function UNSOLICITED_RESPONSE_NEW_SMS(length) {
  Phone.onNewSMS(length);
};
RIL[UNSOLICITED_RESPONSE_NEW_SMS_STATUS_REPORT] = function UNSOLICITED_RESPONSE_NEW_SMS_STATUS_REPORT(length) {
  let info = Buf.readStringList();
  Phone.onNewSMSStatusReport(info);
};
RIL[UNSOLICITED_RESPONSE_NEW_SMS_ON_SIM] = function UNSOLICITED_RESPONSE_NEW_SMS_ON_SIM(length) {
  let info = Buf.readUint32List();
  Phone.onNewSMSOnSIM(message);
};
RIL[UNSOLICITED_ON_USSD] = null;
RIL[UNSOLICITED_ON_USSD_REQUEST] = null;
RIL[UNSOLICITED_NITZ_TIME_RECEIVED] = null;
RIL[UNSOLICITED_SIGNAL_STRENGTH] = function UNSOLICITED_SIGNAL_STRENGTH() {
  this[REQUEST_SIGNAL_STRENGTH]();
};
RIL[UNSOLICITED_DATA_CALL_LIST_CHANGED] = function UNSOLICITED_DATA_CALL_LIST_CHANGED(length) {
  Phone.onDataCallListChanged();
};
RIL[UNSOLICITED_SUPP_SVC_NOTIFICATION] = null;
RIL[UNSOLICITED_STK_SESSION_END] = null;
RIL[UNSOLICITED_STK_PROACTIVE_COMMAND] = null;
RIL[UNSOLICITED_STK_EVENT_NOTIFY] = null;
RIL[UNSOLICITED_STK_CALL_SETUP] = null;
RIL[UNSOLICITED_SIM_SMS_STORAGE_FULL] = null;
RIL[UNSOLICITED_SIM_REFRESH] = null;
RIL[UNSOLICITED_CALL_RING] = function UNSOLICITED_CALL_RING() {
  let info;
  let isCDMA = false; 
  if (isCDMA) {
    info = {
      isPresent:  Buf.readUint32(),
      signalType: Buf.readUint32(),
      alertPitch: Buf.readUint32(),
      signal:     Buf.readUint32()
    };
  }
  Phone.onCallRing(info);
};
RIL[UNSOLICITED_RESPONSE_SIM_STATUS_CHANGED] = function UNSOLICITED_RESPONSE_SIM_STATUS_CHANGED() {
  Phone.onICCStatusChanged();
};
RIL[UNSOLICITED_RESPONSE_CDMA_NEW_SMS] = null;
RIL[UNSOLICITED_RESPONSE_NEW_BROADCAST_SMS] = null;
RIL[UNSOLICITED_CDMA_RUIM_SMS_STORAGE_FULL] = null;
RIL[UNSOLICITED_RESTRICTED_STATE_CHANGED] = null;
RIL[UNSOLICITED_ENTER_EMERGENCY_CALLBACK_MODE] = null;
RIL[UNSOLICITED_CDMA_CALL_WAITING] = null;
RIL[UNSOLICITED_CDMA_OTA_PROVISION_STATUS] = null;
RIL[UNSOLICITED_CDMA_INFO_REC] = null;
RIL[UNSOLICITED_OEM_HOOK_RAW] = null;
RIL[UNSOLICITED_RINGBACK_TONE] = null;
RIL[UNSOLICITED_RESEND_INCALL_MUTE] = null;







let Phone = {

  


  radioState: RADIO_STATE_UNAVAILABLE,

  


  IMEI: null,
  IMEISV: null,
  IMSI: null,
  SMSC: null,

  registrationState: {},
  gprsRegistrationState: {},

  


  operator: null,

  


  basebandVersion: null,

  


  networkSelectionMode: null,

  



  iccStatus: null,

  


  cardState: null,

  


  currentCalls: {},

  


  _muted: true,

  


  currentDataCalls: {},

  


  activeDataRequests: {},

  get muted() {
    return this._muted;
  },

  set muted(val) {
    val = Boolean(val);
    if (this._muted != val) {
      RIL.setMute(val);
      this._muted = val;
    }
  },

  _handleChangedCallState: function _handleChangedCallState(changedCall) {
    let message = {type: "callStateChange",
                   call: {callIndex: changedCall.callIndex,
                          state: changedCall.state,
                          number: changedCall.number,
                          name: changedCall.name}};
    this.sendDOMMessage(message);
  },

  _handleDisconnectedCall: function _handleDisconnectedCall(disconnectedCall) {
    let message = {type: "callDisconnected",
                   call: {callIndex: disconnectedCall.callIndex}};
    this.sendDOMMessage(message);
  },

  




  onRadioStateChanged: function onRadioStateChanged(newState) {
    if (DEBUG) {
      debug("Radio state changed from " + this.radioState + " to " + newState);
    }
    if (this.radioState == newState) {
      
      return;
    }

    let gsm = newState == RADIO_STATE_SIM_NOT_READY        ||
              newState == RADIO_STATE_SIM_LOCKED_OR_ABSENT ||
              newState == RADIO_STATE_SIM_READY;
    let cdma = newState == RADIO_STATE_RUIM_NOT_READY       ||
               newState == RADIO_STATE_RUIM_READY            ||
               newState == RADIO_STATE_RUIM_LOCKED_OR_ABSENT ||
               newState == RADIO_STATE_NV_NOT_READY          ||
               newState == RADIO_STATE_NV_READY;

    
    

    if (this.radioState == RADIO_STATE_UNAVAILABLE &&
        newState != RADIO_STATE_UNAVAILABLE) {
      
      if (gsm) {
        RIL.getIMEI();
        RIL.getIMEISV();
      }
      if (cdma) {
        RIL.getDeviceIdentity();
      }
      Buf.simpleRequest(REQUEST_BASEBAND_VERSION);
      RIL.setScreenState(true);
      this.sendDOMMessage({
        type: "radiostatechange",
        radioState: (newState == RADIO_STATE_OFF) ?
                     GECKO_RADIOSTATE_OFF : GECKO_RADIOSTATE_READY
      });

      
      
      
      if (newState == RADIO_STATE_OFF) {
        RIL.setRadioPower(true);
      }
    }

    if (newState == RADIO_STATE_UNAVAILABLE) {
      
      
      

      this.sendDOMMessage({type: "radiostatechange",
                           radioState: GECKO_RADIOSTATE_UNAVAILABLE});
    }

    if (newState == RADIO_STATE_SIM_READY  ||
        newState == RADIO_STATE_RUIM_READY ||
        newState == RADIO_STATE_NV_READY) {
      
      RIL.getICCStatus();
      this.requestNetworkInfo();
      RIL.getSignalStrength();
      RIL.getSMSCAddress();
      this.sendDOMMessage({type: "cardstatechange",
                           cardState: GECKO_CARDSTATE_READY});
    }
    if (newState == RADIO_STATE_SIM_LOCKED_OR_ABSENT  ||
        newState == RADIO_STATE_RUIM_LOCKED_OR_ABSENT) {
      RIL.getICCStatus();
      this.sendDOMMessage({type: "cardstatechange",
                           cardState: GECKO_CARDSTATE_UNAVAILABLE});
    }

    let wasOn = this.radioState != RADIO_STATE_OFF &&
                this.radioState != RADIO_STATE_UNAVAILABLE;
    let isOn = newState != RADIO_STATE_OFF &&
               newState != RADIO_STATE_UNAVAILABLE;
    if (!wasOn && isOn) {
      
    }
    if (wasOn && !isOn) {
      
    }

    this.radioState = newState;
  },

  onCurrentCalls: function onCurrentCalls(newCalls) {
    
    
    
    for each (let currentCall in this.currentCalls) {
      let newCall;
      if (newCalls) {
        newCall = newCalls[currentCall.callIndex];
        delete newCalls[currentCall.callIndex];
      }

      if (newCall) {
        
        if (newCall.state != currentCall.state) {
          
          currentCall.state = newCall.state;
          this._handleChangedCallState(currentCall);
        }
      } else {
        
        
        delete this.currentCalls[currentCall.callIndex];
        this._handleDisconnectedCall(currentCall);
      }
    }

    
    for each (let newCall in newCalls) {
      if (newCall.isVoice) {
        
        if (newCall.number &&
            newCall.toa == TOA_INTERNATIONAL &&
            newCall.number[0] != "+") {
          newCall.number = "+" + newCall.number;
        }
        
        this.currentCalls[newCall.callIndex] = newCall;
        this._handleChangedCallState(newCall);
      }
    }

    
    
    this.muted = Object.getOwnPropertyNames(this.currentCalls).length == 0;
  },

  onCallStateChanged: function onCallStateChanged() {
    RIL.getCurrentCalls();
  },

  onCallRing: function onCallRing(info) {
    
    
  },

  onNetworkStateChanged: function onNetworkStateChanged() {
    if (DEBUG) debug("Network state changed, re-requesting phone state.");
    this.requestNetworkInfo();
  },

  onICCStatus: function onICCStatus(iccStatus) {
    if (DEBUG) {
      debug("iccStatus: " + JSON.stringify(iccStatus));
    }
    this.iccStatus = iccStatus;

    if ((!iccStatus) || (iccStatus.cardState == CARD_STATE_ABSENT)) {
      if (DEBUG) debug("ICC absent");
      if (this.cardState == GECKO_CARDSTATE_ABSENT) {
        return;
      }
      this.cardState = GECKO_CARDSTATE_ABSENT;
      this.sendDOMMessage({type: "cardstatechange",
                           cardState: this.cardState});
      return;
    }

    if ((this.radioState == RADIO_STATE_OFF) ||
        (this.radioState == RADIO_STATE_UNAVAILABLE) ||
        (this.radioState == RADIO_STATE_SIM_NOT_READY) ||
        (this.radioState == RADIO_STATE_RUIM_NOT_READY) ||
        (this.radioState == RADIO_STATE_NV_NOT_READY) ||
        (this.radioState == RADIO_STATE_NV_READY)) {
      if (DEBUG) debug("ICC not ready");
      if (this.cardState == GECKO_CARDSTATE_NOT_READY) {
        return;
      }
      this.cardState = GECKO_CARDSTATE_NOT_READY;
      this.sendDOMMessage({type: "cardstatechange",
                           cardState: this.cardState});
      return;
    }

    if ((this.radioState == RADIO_STATE_SIM_LOCKED_OR_ABSENT) ||
        (this.radioState == RADIO_STATE_SIM_READY) ||
        (this.radioState == RADIO_STATE_RUIM_LOCKED_OR_ABSENT) ||
        (this.radioState == RADIO_STATE_RUIM_READY)) {
      let app = iccStatus.apps[iccStatus.gsmUmtsSubscriptionAppIndex];
      if (!app) {
        if (DEBUG) {
          debug("Subscription application is not present in iccStatus.");
        }
        if (this.cardState == GECKO_CARDSTATE_ABSENT) {
          return;
        }
        this.cardState = GECKO_CARDSTATE_ABSENT;
        this.sendDOMMessage({type: "cardstatechange",
                             cardState: this.cardState});
        return;
      }

      let newCardState;
      switch (app.app_state) {
        case CARD_APP_STATE_PIN:
          newCardState = GECKO_CARDSTATE_PIN_REQUIRED;
          break;
        case CARD_APP_STATE_PUK:
          newCardState = GECKO_CARDSTATE_PUK_REQUIRED;
          break;
        case CARD_APP_STATE_SUBSCRIPTION_PERSO:
          newCardState = GECKO_CARDSTATE_NETWORK_LOCKED;
          break;
        case CARD_APP_STATE_READY:
          newCardState = GECKO_CARDSTATE_READY;
          break;
        case CARD_APP_STATE_UNKNOWN:
        case CARD_APP_STATE_DETECTED:
        default:
          newCardState = GECKO_CARDSTATE_NOT_READY;
      }

      if (this.cardState == newCardState) {
        return;
      }
      this.cardState = newCardState;
      this.sendDOMMessage({type: "cardstatechange",
                           cardState: this.cardState});
    }
  },

  onICCStatusChanged: function onICCStatusChanged() {
    RIL.getICCStatus();
  },

  onEnterICCPIN: function onEnterICCPIN(response) {
    if (DEBUG) debug("REQUEST_ENTER_SIM_PIN returned " + response);
    
  },

  onChangeICCPIN: function onChangeICCPIN() {
    if (DEBUG) debug("REQUEST_CHANGE_SIM_PIN");
    
  },

  onEnterICCPUK: function onEnterICCPUK(response) {
    if (DEBUG) debug("REQUEST_ENTER_SIM_PUK returned " + response);
    
  },

  onNetworkSelectionMode: function onNetworkSelectionMode(mode) {
    this.networkSelectionMode = mode[0];
  },

  onBasebandVersion: function onBasebandVersion(version) {
    this.basebandVersion = version;
  },

  onIMSI: function onIMSI(imsi) {
    this.IMSI = imsi;
  },

  onIMEI: function onIMEI(imei) {
    this.IMEI = imei;
  },

  onIMEISV: function onIMEISV(imeiSV) {
    this.IMEISV = imeiSV;
  },

  onRegistrationState: function onRegistrationState(state) {
    let rs = this.registrationState;
    let stateChanged = false;

    let regState = RIL.parseInt(state[0], NETWORK_CREG_STATE_UNKNOWN);
    if (rs.regState != regState) {
      rs.regState = regState;
      stateChanged = true;
    }

    let radioTech = RIL.parseInt(state[3], NETWORK_CREG_TECH_UNKNOWN);
    if (rs.radioTech != radioTech) {
      rs.radioTech = radioTech;
      stateChanged = true;
    }

    
    
    let cdma = false;
    if (cdma) {
      let baseStationId = RIL.parseInt(state[4]);
      let baseStationLatitude = RIL.parseInt(state[5]);
      let baseStationLongitude = RIL.parseInt(state[6]);
      if (!baseStationLatitude && !baseStationLongitude) {
        baseStationLatitude = baseStationLongitude = null;
      }
      let cssIndicator = RIL.parseInt(state[7]);
      let systemId = RIL.parseInt(state[8]);
      let networkId = RIL.parseInt(state[9]);
      let roamingIndicator = RIL.parseInt(state[10]);
      let systemIsInPRL = RIL.parseInt(state[11]);
      let defaultRoamingIndicator = RIL.parseInt(state[12]);
      let reasonForDenial = RIL.parseInt(state[13]);
    }

    if (stateChanged) {
      this.sendDOMMessage({type: "registrationstatechange",
                           registrationState: rs});
    }
  },

  onGPRSRegistrationState: function onGPRSRegistrationState(state) {
    let rs = this.gprsRegistrationState;
    let stateChanged = false;

    let regState = RIL.parseInt(state[0], NETWORK_CREG_STATE_UNKNOWN);
    if (rs.regState != regState) {
      rs.regState = regState;
      stateChanged = true;
    }

    let radioTech = RIL.parseInt(state[3], NETWORK_CREG_TECH_UNKNOWN);
    if (rs.radioTech != radioTech) {
      rs.radioTech = radioTech;
      stateChanged = true;
    }

    if (stateChanged) {
      this.sendDOMMessage({type: "gprsregistrationstatechange",
                           gprsRegistrationState: rs});
    }
  },

  onOperator: function onOperator(operator) {
    if (operator.length < 3) {
      if (DEBUG) debug("Expected at least 3 strings for operator.");
    }
    if (!this.operator ||
        this.operator.alphaLong  != operator[0] ||
        this.operator.alphaShort != operator[1] ||
        this.operator.numeric    != operator[2]) {
      this.operator = {alphaLong:  operator[0],
                       alphaShort: operator[1],
                       numeric:    operator[2]};
      this.sendDOMMessage({type: "operatorchange",
                           operator: this.operator});
    }
  },

  onSignalStrength: function onSignalStrength(strength) {
    if (DEBUG) debug("Signal strength " + JSON.stringify(strength));
    this.sendDOMMessage({type: "signalstrengthchange",
                         signalStrength: strength});
  },

  onDial: function onDial() {
  },

  onHangUp: function onHangUp() {
  },

  onAnswerCall: function onAnswerCall() {
  },

  onRejectCall: function onRejectCall() {
  },

  onSetMute: function onSetMute() {
  },

  onSendTone: function onSendTone() {
  },

  onStartTone: function onStartTone() {
  },

  onStopTone: function onStopTone() {
  },

  onGetSMSCAddress: function onGetSMSCAddress(smsc) {
    this.SMSC = smsc;
  },

  onSetSMSCAddress: function onSetSMSCAddress() {
  },

  onSendSMS: function onSendSMS(options) {
    options.type = "sms-sent";
    this.sendDOMMessage(options);
  },

  onNewSMS: function onNewSMS(payloadLength) {
    if (!payloadLength) {
      if (DEBUG) debug("Received empty SMS!");
      
      
      return;
    }
    
    
    let messageStringLength = Buf.readUint32();
    if (DEBUG) debug("Got new SMS, length " + messageStringLength);
    let message = GsmPDUHelper.readMessage();
    if (DEBUG) debug(message);

    
    let delimiter = Buf.readUint16();
    if (!(messageStringLength & 1)) {
      delimiter |= Buf.readUint16();
    }
    if (DEBUG) {
      if (delimiter != 0) {
        debug("Something's wrong, found string delimiter: " + delimiter);
      }
    }

    message.type = "sms-received";
    this.sendDOMMessage(message);

    
    
    RIL.acknowledgeSMS(true, SMS_HANDLED);
  },

  onNewSMSStatusReport: function onNewSMSStatusReport(info) {
    
  },

  onNewSMSOnSIM: function onNewSMSOnSIM(info) {
    
  },

  onAcknowledgeSMS: function onAcknowledgeSMS() {
  },

  onSetupDataCall: function onSetupDataCall(token, cid, ifname, ipaddr,
                                            dns, gw) {
    let options = this.activeDataRequests[token];
    delete this.activeDataRequests[token];

    let datacall = this.currentDataCalls[cid] = {
      active: -1,
      state: GECKO_NETWORK_STATE_CONNECTING,
      cid: cid,
      apn: options.apn,
      ifname: ifname,
      ipaddr: ipaddr,
      dns: dns,
      gw: gw,
    };
    this.sendDOMMessage({type: "datacallstatechange",
                         datacall: datacall});

    
    
    RIL.getDataCallList();
  },

  onDeactivateDataCall: function onDeactivateDataCall(token) {
    let options = this.activeDataRequests[token];
    delete this.activeDataRequests[token];

    let datacall = this.currentDataCalls[options.cid];
    delete this.currentDataCalls[options.cid];
    datacall.state = GECKO_NETWORK_STATE_DISCONNECTED;
    this.sendDOMMessage({type: "datacallstatechange",
                         datacall: datacall});
  },

  onDataCallList: function onDataCallList(datacalls) {
    for each (let currentDataCall in this.currentDataCalls) {
      let newDataCall;
      if (datacalls) {
        newDataCall = datacalls[currentDataCall.cid];
        delete datacalls[currentDataCall.cid];
      }

      if (newDataCall) {
        switch (newDataCall.active) {
          case DATACALL_INACTIVE:
            newDataCall.state = GECKO_NETWORK_STATE_DISCONNECTED;
            break;
          case DATACALL_ACTIVE_DOWN:
            newDataCall.state = GECKO_NETWORK_STATE_SUSPENDED;
            break;
          case DATACALL_ACTIVE_UP:
            newDataCall.state = GECKO_NETWORK_STATE_CONNECTED;
            break;
        }
        if (newDataCall.state != currentDataCall.state) {
          currentDataCall.active = newDataCall.active;
          currentDataCall.state = newDataCall.state;
          this.sendDOMMessage({type: "datacallstatechange",
                               datacall: currentDataCall});
        }
      } else {
        delete this.currentCalls[currentDataCall.callIndex];
        currentDataCall.state = GECKO_NETWORK_STATE_DISCONNECTED;
        this.sendDOMMessage({type: "datacallstatechange",
                             datacall: currentDataCall});
      }
    }

    for each (let datacall in datacalls) {
      if (DEBUG) debug("Unexpected data call: " + JSON.stringify(datacall));
    }
  },

  onDataCallListChanged: function onDataCallListChanged() {
    RIL.getDataCallList();
  },

  












  


  requestNetworkInfo: function requestNetworkInfo() {
    if (DEBUG) debug("Requesting phone state");
    RIL.getRegistrationState();
    RIL.getGPRSRegistrationState(); 
    RIL.getOperator();
    RIL.getNetworkSelectionMode();
  },

  


  enumerateCalls: function enumerateCalls() {
    if (DEBUG) debug("Sending all current calls");
    let calls = [];
    for each (let call in this.currentCalls) {
      calls.push(call);
    }
    this.sendDOMMessage({type: "enumerateCalls", calls: calls});
  },

  enumerateDataCalls: function enumerateDataCalls() {
    let datacall_list = [];
    for each (let datacall in this.currentDataCalls) {
      datacall_list.push(datacall);
    }
    this.sendDOMMessage({type: "datacalllist",
                         datacalls: datacall_list});
  },

  





  dial: function dial(options) {
    RIL.dial(options.number, 0, 0);
  },

  





  sendTone: function sendTone(options) {
    RIL.sendTone(options.dtmfChar);
  },

  





  startTone: function startTone(options) {
    RIL.startTone(options.dtmfChar);
  },

  


  stopTone: function stopTone() {
    RIL.stopTone();
  },

  





  hangUp: function hangUp(options) {
    
    
    RIL.hangUp(options.callIndex);
  },

  





  answerCall: function answerCall(options) {
    
    
    
    
    let call = this.currentCalls[options.callIndex];
    if (call && call.state == CALL_STATE_INCOMING) {
      RIL.answerCall();
    }
  },

  





  rejectCall: function rejectCall(options) {
    
    
    
    
    let call = this.currentCalls[options.callIndex];
    if (call && call.state == CALL_STATE_INCOMING) {
      RIL.rejectCall();
    }
  },

  











  sendSMS: function sendSMS(options) {
    
    if (!this.SMSC) {
      
      
      
      if (DEBUG) {
        debug("Cannot send the SMS. Need to get the SMSC address first.");
      }
      return;
    }
    
    
    
    options.SMSC = this.SMSC;

    
    
    
    options.dcs = PDU_DCS_MSG_CODING_7BITS_ALPHABET;
    options.bodyLengthInOctets = Math.ceil(options.body.length * 7 / 8);
    RIL.sendSMS(options);
  },

  


  setupDataCall: function setupDataCall(options) {
    if (DEBUG) debug("setupDataCall: " + JSON.stringify(options));

    let token = RIL.setupDataCall(options.radioTech, options.apn,
                                  options.user, options.passwd,
                                  options.chappap, options.pdptype);
    this.activeDataRequests[token] = options;
  },

  


  deactivateDataCall: function deactivateDataCall(options) {
    if (!(options.cid in this.currentDataCalls)) {
      return;
    }

    let reason = options.reason || DATACALL_DEACTIVATE_NO_REASON;
    let token = RIL.deactivateDataCall(options.cid, reason);
    this.activeDataRequests[token] = options;

    let datacall = this.currentDataCalls[options.cid];
    datacall.state = GECKO_NETWORK_STATE_DISCONNECTING;
    this.sendDOMMessage({type: "datacallstatechange",
                         datacall: datacall});
  },

  


  getDataCallList: function getDataCallList(options) {
    RIL.getDataCallList();
  },

  


  getFailCauseCode: function getFailCauseCode(options) {
    RIL.getFailCauseCode();
  },

  





  handleDOMMessage: function handleMessage(message) {
    if (DEBUG) debug("Received DOM message " + JSON.stringify(message));
    let method = this[message.type];
    if (typeof method != "function") {
      if (DEBUG) {
        debug("Don't know what to do with message " + JSON.stringify(message));
      }
      return;
    }
    method.call(this, message);
  },

  


  sendDOMMessage: function sendDOMMessage(message) {
    postMessage(message, "*");
  }

};










let GsmPDUHelper = {

  




  readHexNibble: function readHexNibble() {
    let nibble = Buf.readUint16();
    if (nibble >= 48 && nibble <= 57) {
      nibble -= 48; 
    } else if (nibble >= 65 && nibble <= 70) {
      nibble -= 55; 
    } else if (nibble >= 97 && nibble <= 102) {
      nibble -= 87; 
    } else {
      throw "Found invalid nibble during PDU parsing: " +
            String.fromCharCode(nibble);
    }
    return nibble;
  },

  





  writeHexNibble: function writeHexNibble(nibble) {
    nibble &= 0x0f;
    if (nibble < 10) {
      nibble += 48; 
    } else {
      nibble += 55; 
    }
    Buf.writeUint16(nibble);
  },

  




  readHexOctet: function readHexOctet() {
    return (this.readHexNibble() << 4) | this.readHexNibble();
  },

  





  writeHexOctet: function writeHexOctet(octet) {
    this.writeHexNibble(octet >> 4);
    this.writeHexNibble(octet);
  },

  









  octetToBCD: function octetToBCD(octet) {
    return ((octet & 0xf0) <= 0x90) * ((octet >> 4) & 0x0f) +
           ((octet & 0x0f) <= 0x09) * (octet & 0x0f) * 10;
  },

  







  readSwappedNibbleBCD: function readSwappedNibbleBCD(length) {
    let number = 0;
    for (let i = 0; i < length; i++) {
      let octet = this.readHexOctet();
      
      
      if ((octet & 0xf0) == 0xf0) {
        number *= 10;
        number += octet & 0x0f;
        continue;
      }
      number *= 100;
      number += this.octetToBCD(octet);
    }
    return number;
  },

  





  writeSwappedNibbleBCD: function writeSwappedNibbleBCD(data) {
    data = data.toString();
    if (data.length % 2) {
      data += "F";
    }
    for (let i = 0; i < data.length; i += 2) {
      Buf.writeUint16(data.charCodeAt(i + 1));
      Buf.writeUint16(data.charCodeAt(i));
    }
  },

  











  readSeptetsToString: function readSeptetsToString(length) {
    let ret = "";
    let byteLength = Math.ceil(length * 7 / 8);

    let leftOver = 0;
    for (let i = 0; i < byteLength; i++) {
      let octet = this.readHexOctet();
      let shift = (i % 7);
      let leftOver_mask = (0xff << (7 - shift)) & 0xff;
      let septet_mask = (0xff >> (shift + 1));

      let septet = ((octet & septet_mask) << shift) | leftOver;
      ret += PDU_ALPHABET_7BIT_DEFAULT[septet];
      leftOver = (octet & leftOver_mask) >> (7 - shift);

      
      if (shift == 6) {
        ret += PDU_ALPHABET_7BIT_DEFAULT[leftOver];
        leftOver = 0;
      }
    }
    if (ret.length != length) {
      ret = ret.slice(0, length);
    }
    return ret;
  },

  writeStringAsSeptets: function writeStringAsSeptets(message) {
    let right = 0;
    for (let i = 0; i < message.length + 1; i++) {
      let shift = (i % 8);
      let septet;
      if (i < message.length) {
        septet = PDU_ALPHABET_7BIT_DEFAULT.indexOf(message[i]);
      } else {
        septet = 0;
      }
      if (septet == -1) {
        if (DEBUG) debug("Fffff, "  + message[i] + " not in 7 bit alphabet!");
        septet = 0;
      }
      if (shift == 0) {
        
        
        right = septet;
        continue;
      }

      let left_mask = 0xff >> (8 - shift);
      let right_mask = (0xff << shift) & 0xff;
      let left = (septet & left_mask) << (8 - shift);
      let octet = left | right;
      this.writeHexOctet(left | right);
      right = (septet & right_mask) >> shift;
    }
  },

  







  readUCS2String: function readUCS2String(length) {
    
  },

  





  writeUCS2String: function writeUCS2String(message) {
    
  },

  





  readUserData: function readUserData(length, codingScheme) {
    if (DEBUG) {
      debug("Reading " + length + " bytes of user data.");
      debug("Coding scheme: " + codingScheme);
    }
    
    let encoding = PDU_DCS_MSG_CODING_7BITS_ALPHABET;
    switch (codingScheme & 0xC0) {
      case 0x0:
        
        switch (codingScheme & 0x0C) {
          case 0x4:
            encoding = PDU_DCS_MSG_CODING_8BITS_ALPHABET;
            break;
          case 0x8:
            encoding = PDU_DCS_MSG_CODING_16BITS_ALPHABET;
            break;
        }
        break;
      case 0xC0:
        
        switch (codingScheme & 0x30) {
          case 0x20:
            encoding = PDU_DCS_MSG_CODING_16BITS_ALPHABET;
            break;
          case 0x30:
            if (!codingScheme & 0x04) {
              encoding = PDU_DCS_MSG_CODING_8BITS_ALPHABET;
            }
            break;
        }
        break;
      default:
        
        break;
    }

    if (DEBUG) debug("PDU: message encoding is " + encoding + " bit.");
    switch (encoding) {
      case PDU_DCS_MSG_CODING_7BITS_ALPHABET:
        
        
        if (length > PDU_MAX_USER_DATA_7BIT) {
          if (DEBUG) debug("PDU error: user data is too long: " + length);
          return null;
        }
        return this.readSeptetsToString(length);
      case PDU_DCS_MSG_CODING_8BITS_ALPHABET:
        
        return null;
      case PDU_DCS_MSG_CODING_16BITS_ALPHABET:
        return this.readUCS2String(length);
    }
    return null;
  },

  





  readMessage: function readMessage() {
    
    let msg = {
      SMSC:      null,
      reference: null,
      sender:    null,
      body:      null,
      validity:  null,
      timestamp: null
    };

    
    let smscLength = this.readHexOctet();
    if (smscLength > 0) {
      let smscTypeOfAddress = this.readHexOctet();
      
      msg.SMSC = this.readSwappedNibbleBCD(smscLength - 1).toString();
      if ((smscTypeOfAddress >> 4) == (PDU_TOA_INTERNATIONAL >> 4)) {
        msg.SMSC = '+' + msg.SMSC;
      }
    }

    
    let firstOctet = this.readHexOctet();
    
    let isSmsSubmit = firstOctet & PDU_MTI_SMS_SUBMIT;
    if (isSmsSubmit) {
      msg.reference = this.readHexOctet(); 
    }

    
    
    let senderAddressLength = this.readHexOctet();
    if (senderAddressLength <= 0) {
      if (DEBUG) debug("PDU error: invalid sender address length: " + senderAddressLength);
      return null;
    }
    
    let senderTypeOfAddress = this.readHexOctet();
    if (senderAddressLength % 2 == 1) {
      senderAddressLength += 1;
    }
    if (DEBUG) debug("PDU: Going to read sender address: " + senderAddressLength);
    msg.sender = this.readSwappedNibbleBCD(senderAddressLength / 2).toString();
    if (msg.sender.length <= 0) {
      if (DEBUG) debug("PDU error: no sender number provided");
      return null;
    }
    if ((senderTypeOfAddress >> 4) == (PDU_TOA_INTERNATIONAL >> 4)) {
      msg.sender = '+' + msg.sender;
    }

    
    let protocolIdentifier = this.readHexOctet();

    
    let dataCodingScheme = this.readHexOctet();

    
    
    if (isSmsSubmit) {
      
      
      
      
      
      
      
      
      
      
      if (firstOctet & (PDU_VPF_ABSOLUTE | PDU_VPF_RELATIVE | PDU_VPF_ENHANCED)) {
        msg.validity = this.readHexOctet();
      }
      
    } else {
      
      let year   = this.readSwappedNibbleBCD(1) + PDU_TIMESTAMP_YEAR_OFFSET;
      let month  = this.readSwappedNibbleBCD(1) - 1;
      let day    = this.readSwappedNibbleBCD(1) - 1;
      let hour   = this.readSwappedNibbleBCD(1) - 1;
      let minute = this.readSwappedNibbleBCD(1) - 1;
      let second = this.readSwappedNibbleBCD(1) - 1;
      msg.timestamp = Date.UTC(year, month, day, hour, minute, second);

      
      
      let tzOctet = this.readHexOctet();
      let tzOffset = this.octetToBCD(tzOctet & ~0x08) * 15 * 60 * 1000;
      if (tzOctet & 0x08) {
        msg.timestamp -= tzOffset;
      } else {
        msg.timestamp += tzOffset;
      }
    }

    
    let userDataLength = this.readHexOctet();

    
    if (userDataLength > 0) {
      msg.body = this.readUserData(userDataLength, dataCodingScheme);
    }

    return msg;
  },

  
















  writeMessage: function writeMessage(address,
                                      userData,
                                      dcs,
                                      userDataLengthInOctets) {
    
    
    
    
    
    
    
    
    
    

    let addressFormat = PDU_TOA_ISDN; 
    if (address[0] == '+') {
      addressFormat = PDU_TOA_INTERNATIONAL | PDU_TOA_ISDN; 
      address = address.substring(1);
    }
    
    let validity = 0;

    let pduOctetLength = 4 + 
                         Math.ceil(address.length / 2) +
                         3 + 
                         userDataLengthInOctets;
    if (validity) {
      
    }

    
    
    Buf.writeUint32(pduOctetLength * 2);

    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    let firstOctet = PDU_MTI_SMS_SUBMIT;

    
    if (validity) {
      
    }
    let udhi = ""; 
    if (udhi) {
      firstOctet |= PDU_UDHI;
    }
    this.writeHexOctet(firstOctet);

    
    this.writeHexOctet(0x00);

    
    this.writeHexOctet(address.length);
    this.writeHexOctet(addressFormat);
    this.writeSwappedNibbleBCD(address);

    
    this.writeHexOctet(0x00);

    
    
    this.writeHexOctet(dcs);

    
    if (validity) {
      this.writeHexOctet(validity);
    }

    
    let userDataLength = userData.length;
    if (dcs == PDU_DCS_MSG_CODING_16BITS_ALPHABET) {
      userDataLength = userData.length * 2;
    }
    this.writeHexOctet(userDataLength);
    switch (dcs) {
      case PDU_DCS_MSG_CODING_7BITS_ALPHABET:
        this.writeStringAsSeptets(userData);
        break;
      case PDU_DCS_MSG_CODING_8BITS_ALPHABET:
        
        break;
      case PDU_DCS_MSG_CODING_16BITS_ALPHABET:
        this.writeUCS2String(userData);
        break;
    }

    
    
    Buf.writeUint16(0);
    Buf.writeUint16(0);
  }
};





if (!this.debug) {
  
  this.debug = function debug(message) {
    dump("RIL Worker: " + message + "\n");
  };
}



Buf.init();

function onRILMessage(data) {
  Buf.processIncoming(data);
};

onmessage = function onmessage(event) {
  Phone.handleDOMMessage(event.data);
};

onerror = function onerror(event) {
  debug("RIL Worker error" + event.message + "\n");
};
