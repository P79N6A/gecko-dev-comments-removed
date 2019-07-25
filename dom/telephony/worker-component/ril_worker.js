




























































"use strict";

importScripts("ril_consts.js");

const DEBUG = true;

const INT32_MAX   = 2147483647;
const UINT8_SIZE  = 1;
const UINT16_SIZE = 2;
const UINT32_SIZE = 4;
const PARCEL_SIZE_SIZE = UINT32_SIZE;

const RESPONSE_TYPE_SOLICITED = 0;
const RESPONSE_TYPE_UNSOLICITED = 1;













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

    
    
    this.currentParcelSize = 0;

    
    this.token = 1;

    
    
    this.tokenRequestMap = {};
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

  





  readUint8: function readUint8() {
    let value = this.incomingBytes[this.incomingReadIndex];
    this.incomingReadIndex = (this.incomingReadIndex + 1) %
                             this.INCOMING_BUFFER_LENGTH;
    return value;
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
    return this.readUint8() << 24 | this.readUint8() << 16 |
           this.readUint8() <<  8 | this.readUint8();
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
        this.processParcel();
      } catch (ex) {
        if (DEBUG) debug("Parcel handling threw " + ex);
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
      this.currentParcelSize = 0;
    }
  },

  



  processParcel: function processParcel() {
    let response_type = this.readUint32();
    let length = this.readIncoming - 2 * UINT32_SIZE;

    let request_type;
    if (response_type == RESPONSE_TYPE_SOLICITED) {
      let token = this.readUint32();
      let error = this.readUint32();
      request_type = this.tokenRequestMap[token];
      if (error) {
        
        debug("Received error " + error + " for solicited parcel type " +
              request_type);
        return;
      }
      debug("Solicited response for request type " + request_type +
            ", token " + token);
      delete this.tokenRequestMap[token];
    } else if (response_type == RESPONSE_TYPE_UNSOLICITED) {
      request_type = this.readUint32();
      debug("Unsolicited response for request type " + request_type);
    } else {
      debug("Unknown response type: " + response_type);
      return;
    }

    RIL.handleParcel(request_type, length);
  },

  





  newParcel: function newParcel(type) {
    
    this.outgoingIndex = PARCEL_SIZE_SIZE;
    this.writeUint32(type);
    let token = this.token;
    this.writeUint32(token);
    this.tokenRequestMap[token] = type;
    this.token++;
    return token;
  },

  



  sendParcel: function sendParcel() {
    
    
    
    let parcelSize = this.outgoingIndex - PARCEL_SIZE_SIZE;
    this.writeParcelSize(parcelSize);

    
    
    let parcel = this.outgoingBytes.subarray(0, this.outgoingIndex);
    debug("Outgoing parcel: " + Array.slice(parcel));
    postRILMessage(parcel);
    this.outgoingIndex = PARCEL_SIZE_SIZE;
  },

  simpleRequest: function simpleRequest(type) {
    this.newParcel(type);
    this.sendParcel();
  }
};








let RIL = {

  




  getICCStatus: function getICCStatus() {
    Buf.simpleRequest(REQUEST_GET_SIM_STATUS);
  },

  







  enterICCPIN: function enterICCPIN(pin) {
    Buf.newParcel(REQUEST_ENTER_SIM_PIN);
    Buf.writeUint32(1);
    Buf.writeString(pin);
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

  







  sendSMS: function sendSMS(smscPDU, pdu) {
    let token = Buf.newParcel(REQUEST_SEND_SMS);
    
    
    
    
    Buf.writeUint32(2);
    Buf.writeString(smscPDU);
    Buf.writeString(pdu);
    Buf.sendParcel();
  },

  





  handleParcel: function handleParcel(request_type, length) {
    let method = this[request_type];
    if (typeof method == "function") {
      debug("Handling parcel as " + method.name);
      method.call(this, length);
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
RIL[REQUEST_ENTER_SIM_PUK] = null;
RIL[REQUEST_ENTER_SIM_PIN2] = null;
RIL[REQUEST_ENTER_SIM_PUK2] = null;
RIL[REQUEST_CHANGE_SIM_PIN] = null;
RIL[REQUEST_CHANGE_SIM_PIN2] = null;
RIL[REQUEST_ENTER_NETWORK_DEPERSONALIZATION] = null;
RIL[REQUEST_GET_CURRENT_CALLS] = function REQUEST_GET_CURRENT_CALLS() {
  let calls = [];
  let calls_length = Buf.readUint32();

  for (let i = 0; i < calls_length; i++) {
    let dc = {
      state:              Buf.readUint32(), 
      index:              Buf.readUint32(),
      toa:                Buf.readUint32(),
      isMpty:             Boolean(Buf.readUint32()),
      isMT:               Boolean(Buf.readUint32()),
      als:                Buf.readUint32(),
      isVoice:            Boolean(Buf.readUint32()),
      isVoicePrivacy:     Boolean(Buf.readUint32()),
      somethingOrOther:   Buf.readUint32(), 
      number:             Buf.readString(), 
      numberPresentation: Buf.readUint32(), 
      name:               Buf.readString(),
      namePresentation:   Buf.readUint32(),
      uusInfo:            null
    };
    let uusInfoPresent = Buf.readUint32();
    if (uusInfoPresent == 1) {
      dc.uusInfo = {
        type:     Buf.readUint32(),
        dcs:      Buf.readUint32(),
        userData: null 
      };
    }
    calls.push(dc);
  }

  Phone.onCurrentCalls(calls);
};
RIL[REQUEST_DIAL] = null;
RIL[REQUEST_GET_IMSI] = function REQUEST_GET_IMSI(length) {
  let imsi = Buf.readString(length);
  Phone.onIMSI(imsi);
};
RIL[REQUEST_HANGUP] = null;
RIL[REQUEST_HANGUP_WAITING_OR_BACKGROUND] = null;
RIL[REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND] = null;
RIL[REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE] = null;
RIL[REQUEST_SWITCH_HOLDING_AND_ACTIVE] = null;
RIL[REQUEST_CONFERENCE] = null;
RIL[REQUEST_UDUB] = null;
RIL[REQUEST_LAST_CALL_FAIL_CAUSE] = null;
RIL[REQUEST_SIGNAL_STRENGTH] = function REQUEST_SIGNAL_STRENGTH() {
  let strength = {
    
    gsmSignalStrength: Buf.readUint32(),
    
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
RIL[REQUEST_DTMF] = null;
RIL[REQUEST_SEND_SMS] = function REQUEST_SEND_SMS() {
  let messageRef = Buf.readUint32();
  let ackPDU = p.readString();
  let errorCode = p.readUint32();
  Phone.onSendSMS(messageRef, ackPDU, errorCode);
};
RIL[REQUEST_SEND_SMS_EXPECT_MORE] = null;
RIL[REQUEST_SETUP_DATA_CALL] = null;
RIL[REQUEST_SIM_IO] = null;
RIL[REQUEST_SEND_USSD] = null;
RIL[REQUEST_CANCEL_USSD] = null;
RIL[REQUEST_GET_CLIR] = null;
RIL[REQUEST_SET_CLIR] = null;
RIL[REQUEST_QUERY_CALL_FORWARD_STATUS] = null;
RIL[REQUEST_SET_CALL_FORWARD] = null;
RIL[REQUEST_QUERY_CALL_WAITING] = null;
RIL[REQUEST_SET_CALL_WAITING] = null;
RIL[REQUEST_SMS_ACKNOWLEDGE] = null;
RIL[REQUEST_GET_IMEI] = function REQUEST_GET_IMEI() {
  let imei = Buf.readString();
  Phone.onIMEI(imei);
};
RIL[REQUEST_GET_IMEISV] = function REQUEST_GET_IMEISV() {
  let imeiSV = Buf.readString();
  Phone.onIMEISV(imeiSV);
};
RIL[REQUEST_ANSWER] = null;
RIL[REQUEST_DEACTIVATE_DATA_CALL] = null;
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
RIL[REQUEST_DTMF_START] = null;
RIL[REQUEST_DTMF_STOP] = null;
RIL[REQUEST_BASEBAND_VERSION] = function REQUEST_BASEBAND_VERSION() {
  let version = Buf.readString();
  Phone.onBasebandVersion(version);
},
RIL[REQUEST_SEPARATE_CONNECTION] = null;
RIL[REQUEST_SET_MUTE] = null;
RIL[REQUEST_GET_MUTE] = null;
RIL[REQUEST_QUERY_CLIP] = null;
RIL[REQUEST_LAST_DATA_CALL_FAIL_CAUSE] = null;
RIL[REQUEST_DATA_CALL_LIST] = null;
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
RIL[REQUEST_GET_SMSC_ADDRESS] = null;
RIL[REQUEST_SET_SMSC_ADDRESS] = null;
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
RIL[UNSOLICITED_RESPONSE_NEW_SMS] = null;
RIL[UNSOLICITED_RESPONSE_NEW_SMS_STATUS_REPORT] = null;
RIL[UNSOLICITED_RESPONSE_NEW_SMS_ON_SIM] = null;
RIL[UNSOLICITED_ON_USSD] = null;
RIL[UNSOLICITED_ON_USSD_REQUEST] = null;
RIL[UNSOLICITED_NITZ_TIME_RECEIVED] = null;
RIL[UNSOLICITED_SIGNAL_STRENGTH] = function UNSOLICITED_SIGNAL_STRENGTH() {
  this[REQUEST_SIGNAL_STRENGTH]();
};
RIL[UNSOLICITED_DATA_CALL_LIST_CHANGED] = null;
RIL[UNSOLICITED_SUPP_SVC_NOTIFICATION] = null;
RIL[UNSOLICITED_STK_SESSION_END] = null;
RIL[UNSOLICITED_STK_PROACTIVE_COMMAND] = null;
RIL[UNSOLICITED_STK_EVENT_NOTIFY] = null;
RIL[UNSOLICITED_STK_CALL_SETUP] = null;
RIL[UNSOLICITED_SIM_SMS_STORAGE_FULL] = null;
RIL[UNSOLICITED_SIM_REFRESH] = null;
RIL[UNSOLICITED_CALL_RING] = null;
RIL[UNSOLICITED_RESPONSE_SIM_STATUS_CHANGED] = null;
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

  


  operator: null,

  


  basebandVersion: null,

  


  networkSelectionMode: null,

  


  iccStatus: null,

  


  calls: null,

  




  onRadioStateChanged: function onRadioStateChanged(newState) {
    debug("Radio state changed from " + this.radioState + " to " + newState);
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
        radioState: (newState == RADIO_STATE_OFF) ? "off" : "ready"
      });

      
      
      
      if (newState == RADIO_STATE_OFF) {
        RIL.setRadioPower(true);
      }
    }

    if (newState == RADIO_STATE_UNAVAILABLE) {
      
      
      

      this.sendDOMMessage({type: "radiostatechange",
                           radioState: "unavailable"});
    }

    if (newState == RADIO_STATE_SIM_READY  ||
        newState == RADIO_STATE_RUIM_READY ||
        newState == RADIO_STATE_NV_READY) {
      
      RIL.getICCStatus();
      this.requestNetworkInfo();
      RIL.getSignalStrength();
      this.sendDOMMessage({type: "cardstatechange",
                           cardState: "ready"});
    }
    if (newState == RADIO_STATE_SIM_LOCKED_OR_ABSENT  ||
        newState == RADIO_STATE_RUIM_LOCKED_OR_ABSENT) {
      RIL.getICCStatus();
      this.sendDOMMessage({type: "cardstatechange",
                           cardState: "unavailable"});
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

  onCurrentCalls: function onCurrentCalls(calls) {
    debug("onCurrentCalls");
    debug(calls);
    
    this.sendDOMMessage({type: "callstatechange", callState: calls});
  },

  onCallStateChanged: function onCallStateChanged() {
    RIL.getCurrentCalls();
  },

  onNetworkStateChanged: function onNetworkStateChanged() {
    debug("Network state changed, re-requesting phone state.");
    this.requestNetworkInfo();
  },

  onICCStatus: function onICCStatus(iccStatus) {
    debug("SIM card state is " + iccStatus.cardState);
    debug("Universal PIN state is " + iccStatus.universalPINState);
    debug(iccStatus);
    
    this.iccStatus = iccStatus; 
  },

  onEnterICCPIN: function onEnterICCPIN(response) {
    debug("REQUEST_ENTER_SIM_PIN returned " + response);
    
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

  onRegistrationState: function onRegistrationState(newState) {
    this.registrationState = newState;
  },

  onGPRSRegistrationState: function onGPRSRegistrationState(newState) {
    this.gprsRegistrationState = newState;
  },

  onOperator: function onOperator(operator) {
    if (operator.length < 3) {
      debug("Expected at least 3 strings for operator.");
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
    debug("Signal strength " + JSON.stringify(strength));
    this.sendDOMMessage({type: "signalstrengthchange",
                         signalStrength: strength});
  },

  onSendSMS: function onSendSMS(messageRef, ackPDU, errorCode) {
    
  },


  












  


  requestNetworkInfo: function requestNetworkInfo() {
    if (DEBUG) debug("Requesting phone state");
    RIL.getRegistrationState();
    RIL.getGPRSRegistrationState(); 
    RIL.getOperator();
    RIL.getNetworkSelectionMode();
  },

  





  dial: function dial(options) {
    RIL.dial(options.number, 0, 0);
  },

  







  sendSMS: function sendSMS(options) {
    
    let smscPDU = "";
    let pdu = "";
    RIL.sendSMS(smscPDU, pdu);
  },

  





  handleDOMMessage: function handleMessage(message) {
    if (DEBUG) debug("Received DOM message " + JSON.stringify(message));
    let method = this[message.type];
    if (typeof method != "function") {
      debug("Don't know what to do with message " + JSON.stringify(message));
      return;
    }
    method.call(this, message);
  },

  


  sendDOMMessage: function sendDOMMessage(message) {
    postMessage(message, "*");
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
