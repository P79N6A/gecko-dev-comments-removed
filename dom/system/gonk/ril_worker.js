





























































"use strict";

importScripts("ril_consts.js", "systemlibs.js");




let DEBUG;

const INT32_MAX   = 2147483647;
const UINT8_SIZE  = 1;
const UINT16_SIZE = 2;
const UINT32_SIZE = 4;
const PARCEL_SIZE_SIZE = UINT32_SIZE;

const PDU_HEX_OCTET_SIZE = 4;

const DEFAULT_EMERGENCY_NUMBERS = ["112", "911"];

let RILQUIRKS_CALLSTATE_EXTRA_UINT32 = false;
let RILQUIRKS_DATACALLSTATE_DOWN_IS_UP = false;


let RILQUIRKS_V5_LEGACY = true;
let RILQUIRKS_REQUEST_USE_DIAL_EMERGENCY_CALL = false;
let RILQUIRKS_MODEM_DEFAULTS_TO_EMERGENCY_MODE = false;













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

  





  






  ensureIncomingAvailable: function ensureIncomingAvailable(index) {
    if (index >= this.currentParcelSize) {
      throw new Error("Trying to read data beyond the parcel end!");
    } else if (index < 0) {
      throw new Error("Trying to read data before the parcel begin!");
    }
  },

  





  seekIncoming: function seekIncoming(offset) {
    
    let cur = this.currentParcelSize - this.readAvailable;

    let newIndex = cur + offset;
    this.ensureIncomingAvailable(newIndex);

    
    
    
    
    
    this.readAvailable = this.currentParcelSize - newIndex;

    
    if (this.incomingReadIndex < cur) {
      
      newIndex += this.INCOMING_BUFFER_LENGTH;
    }
    newIndex += (this.incomingReadIndex - cur);
    newIndex %= this.INCOMING_BUFFER_LENGTH;
    this.incomingReadIndex = newIndex;
  },

  readUint8Unchecked: function readUint8Unchecked() {
    let value = this.incomingBytes[this.incomingReadIndex];
    this.incomingReadIndex = (this.incomingReadIndex + 1) %
                             this.INCOMING_BUFFER_LENGTH;
    return value;
  },

  readUint8: function readUint8() {
    
    let cur = this.currentParcelSize - this.readAvailable;
    this.ensureIncomingAvailable(cur);

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
      delete this.tokenRequestMap[token];
      request_type = options.rilRequestType;

      options.rilRequestError = error;
      if (DEBUG) {
        debug("Solicited response for request type " + request_type +
              ", token " + token + ", error " + error);
      }
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
    options.rilRequestError = null;
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

  simpleRequest: function simpleRequest(type, options) {
    this.newParcel(type, options);
    this.sendParcel();
  }
};









let RIL = {

  


  radioState: GECKO_RADIOSTATE_UNAVAILABLE,

  



  iccStatus: null,

  


  cardState: null,

  


  IMEI: null,
  IMEISV: null,
  SMSC: null,

  


  iccInfo: {},

  voiceRegistrationState: {},
  dataRegistrationState: {},

  


  operator: null,

  


  basebandVersion: null,

  


  networkSelectionMode: null,

  


  currentCalls: {},

  


  currentDataCalls: {},

  




  _receivedSmsSegmentsMap: {},

  


  _pendingSentSmsMap: {},

  


  _muted: true,
  get muted() {
    return this._muted;
  },
  set muted(val) {
    val = Boolean(val);
    if (this._muted != val) {
      this.setMute(val);
      this._muted = val;
    }
  },


  




  rilQuirksInitialized: false,
  initRILQuirks: function initRILQuirks() {
    if (this.rilQuirksInitialized) {
      return;
    }

    let ril_impl = libcutils.property_get("gsm.version.ril-impl");
    if (DEBUG) debug("Detected RIL implementation " + ril_impl);
    switch (ril_impl) {
      case "Samsung RIL(IPC) v2.0":
        
        
        let model_id = libcutils.property_get("ril.model_id");
        if (DEBUG) debug("Detected RIL model " + model_id);
        if (model_id == "I9100") {
          if (DEBUG) {
            debug("Detected I9100, enabling " +
                  "RILQUIRKS_DATACALLSTATE_DOWN_IS_UP, " +
                  "RILQUIRKS_REQUEST_USE_DIAL_EMERGENCY_CALL.");
          }
          RILQUIRKS_DATACALLSTATE_DOWN_IS_UP = true;
          RILQUIRKS_REQUEST_USE_DIAL_EMERGENCY_CALL = true;
          if (RILQUIRKS_V5_LEGACY) {
            if (DEBUG) debug("...and RILQUIRKS_CALLSTATE_EXTRA_UINT32");
            RILQUIRKS_CALLSTATE_EXTRA_UINT32 = true;
          }
        }
        if (model_id == "I9023" || model_id == "I9020") {
          if (DEBUG) {
            debug("Detected I9020/I9023, enabling " +
                  "RILQUIRKS_DATACALLSTATE_DOWN_IS_UP");
          }
          RILQUIRKS_DATACALLSTATE_DOWN_IS_UP = true;
        }
        break;
      case "Qualcomm RIL 1.0":
        if (DEBUG) {
          debug("Detected Qualcomm RIL 1.0, " +
                "disabling RILQUIRKS_V5_LEGACY and " +
                "enabling RILQUIRKS_MODEM_DEFAULTS_TO_EMERGENCY_MODE.");
        }
        RILQUIRKS_V5_LEGACY = false;
        RILQUIRKS_MODEM_DEFAULTS_TO_EMERGENCY_MODE = true;
        break;
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

  





  enterICCPIN: function enterICCPIN(options) {
    Buf.newParcel(REQUEST_ENTER_SIM_PIN);
    Buf.writeUint32(1);
    Buf.writeString(options.pin);
    Buf.sendParcel();
  },

  







  changeICCPIN: function changeICCPIN(options) {
    Buf.newParcel(REQUEST_CHANGE_SIM_PIN);
    Buf.writeUint32(2);
    Buf.writeString(options.oldPin);
    Buf.writeString(options.newPin);
    Buf.sendParcel();
  },

  








   enterICCPUK: function enterICCPUK(options) {
     Buf.newParcel(REQUEST_ENTER_SIM_PUK);
     Buf.writeUint32(2);
     Buf.writeString(options.puk);
     Buf.writeString(options.newPin);
     Buf.sendParcel();
   },

  



















  iccIO: function iccIO(options) {
    let token = Buf.newParcel(REQUEST_SIM_IO, options);
    Buf.writeUint32(options.command);
    Buf.writeUint32(options.fileId);
    Buf.writeString(options.pathId);
    Buf.writeUint32(options.p1);
    Buf.writeUint32(options.p2);
    Buf.writeUint32(options.p3);
    Buf.writeString(options.data);
    if (options.pin2 != null) {
      Buf.writeString(options.pin2);
    }
    Buf.sendParcel();
  },

  


  fetchICCRecords: function fetchICCRecords() {
    this.getIMSI();
    this.getMSISDN();
    this.getAD();
    this.getUST();
  },

  


  _handleICCInfoChange: function _handleICCInfoChange() {
    this.iccInfo.type = "iccinfochange";
    this.sendDOMMessage(this.iccInfo);
  },

  getIMSI: function getIMSI() {
    Buf.simpleRequest(REQUEST_GET_IMSI);
  },

  


  getMSISDN: function getMSISDN() {
    function callback() {
      let length = Buf.readUint32();
      
      let recordLength = length / 2;
      
      Buf.seekIncoming((recordLength - MSISDN_FOOTER_SIZE_BYTES) *
                        PDU_HEX_OCTET_SIZE);

      
      let len = GsmPDUHelper.readHexOctet();
      if (len > MSISDN_MAX_NUMBER_SIZE_BYTES) {
        debug("ICC_EF_MSISDN: invalid length of BCD number/SSC contents - " + len);
        return;
      }
      this.iccInfo.MSISDN = GsmPDUHelper.readAddress(len);
      let delimiter = Buf.readUint16();
      if (!(length & 1)) {
        delimiter |= Buf.readUint16();
      }
      if (DEBUG) {
        if (delimiter != 0) {
          debug("Something's wrong, found string delimiter: " + delimiter);
        }
      }

      if (DEBUG) debug("MSISDN: " + this.iccInfo.MSISDN);
      if (this.iccInfo.MSISDN) {
        this._handleICCInfoChange();
      }
    }

    this.iccIO({
      command:   ICC_COMMAND_GET_RESPONSE,
      fileId:    ICC_EF_MSISDN,
      pathId:    EF_PATH_MF_SIM + EF_PATH_DF_TELECOM,
      p1:        0, 
      p2:        0, 
      p3:        GET_RESPONSE_EF_SIZE_BYTES,
      data:      null,
      pin2:      null,
      type:      EF_TYPE_LINEAR_FIXED,
      callback:  callback,
    });
  },

  


  getAD: function getAD() {
    function callback() {
      let length = Buf.readUint32();
      
      let len = length / 2;
      this.iccInfo.AD = GsmPDUHelper.readHexOctetArray(len);
      let delimiter = Buf.readUint16();
      if (!(length & 1)) {
        delimiter |= Buf.readUint16();
      }
      if (DEBUG) {
        if (delimiter != 0) {
          debug("Something's wrong, found string delimiter: " + delimiter);
        }
      }

      if (DEBUG) {
        let str = "";
        for (let i = 0; i < this.iccInfo.AD.length; i++) {
          str += this.iccInfo.AD[i] + ", ";
        }
        debug("AD: " + str);
      }

      if (this.iccInfo.IMSI) {
        
        this.iccInfo.MCC = this.iccInfo.IMSI.substr(0,3);
        
        this.iccInfo.MNC = this.iccInfo.IMSI.substr(3, this.iccInfo.AD[3]);
        if (DEBUG) debug("MCC: " + this.iccInfo.MCC + " MNC: " + this.iccInfo.MNC);
        this._handleICCInfoChange();
      }
    }

    this.iccIO({
      command:   ICC_COMMAND_GET_RESPONSE,
      fileId:    ICC_EF_AD,
      pathId:    EF_PATH_MF_SIM + EF_PATH_DF_GSM,
      p1:        0, 
      p2:        0, 
      p3:        GET_RESPONSE_EF_SIZE_BYTES,
      data:      null,
      pin2:      null,
      type:      EF_TYPE_TRANSPARENT,
      callback:  callback,
    });
  },

  








  isUSTServiceAvailable: function isUSTServiceAvailable(service) {
    service -= 1;
    let index = service / 8;
    let bitmask = 1 << (service % 8);
    return this.UST && (index < this.UST.length) && (this.UST[index] & bitmask);
  },

  


  getUST: function getUST() {
    function callback() {
      let length = Buf.readUint32();
      
      let len = length / 2;
      this.iccInfo.UST = GsmPDUHelper.readHexOctetArray(len);
      let delimiter = Buf.readUint16();
      if (!(length & 1)) {
        delimiter |= Buf.readUint16();
      }
      if (DEBUG) {
        if (delimiter != 0) {
          debug("Something's wrong, found string delimiter: " + delimiter);
        }
      }
      
      if (DEBUG) {
        let str = "";
        for (let i = 0; i < this.iccInfo.UST.length; i++) {
          str += this.iccInfo.UST[i] + ", ";
        }
        debug("UST: " + str);
      }
    }

    this.iccIO({
      command:   ICC_COMMAND_GET_RESPONSE,
      fileId:    ICC_EF_UST,
      pathId:    EF_PATH_MF_SIM + EF_PATH_DF_GSM,
      p1:        0, 
      p2:        0, 
      p3:        GET_RESPONSE_EF_SIZE_BYTES,
      data:      null,
      pin2:      null,
      type:      EF_TYPE_TRANSPARENT,
      callback:  callback,
    });
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

  getVoiceRegistrationState: function getVoiceRegistrationState() {
    Buf.simpleRequest(REQUEST_VOICE_REGISTRATION_STATE);
  },

  getDataRegistrationState: function getDataRegistrationState() {
    Buf.simpleRequest(REQUEST_DATA_REGISTRATION_STATE);
  },

  getOperator: function getOperator() {
    Buf.simpleRequest(REQUEST_OPERATOR);
  },

  getNetworkSelectionMode: function getNetworkSelectionMode() {
    Buf.simpleRequest(REQUEST_QUERY_NETWORK_SELECTION_MODE);
  },

  setNetworkSelectionAutomatic: function setNetworkSelectionAutomatic() {
    Buf.simpleRequest(REQUEST_SET_NETWORK_SELECTION_AUTOMATIC);
  },

  





  setPreferredNetworkType: function setPreferredNetworkType(network_type) {
    Buf.newParcel(REQUEST_SET_PREFERRED_NETWORK_TYPE);
    Buf.writeUint32(network_type);
    Buf.sendParcel();
  },

  


  requestNetworkInfo: function requestNetworkInfo() {
    if (DEBUG) debug("Requesting phone state");
    this.getVoiceRegistrationState();
    this.getDataRegistrationState(); 
    this.getOperator();
    this.getNetworkSelectionMode();
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

  getBasebandVersion: function getBasebandVersion() {
    Buf.simpleRequest(REQUEST_BASEBAND_VERSION);
  },

  









  dial: function dial(options) {
    let dial_request_type = REQUEST_DIAL;
    if (this.voiceRegistrationState.emergencyCallsOnly) {
      if (!this._isEmergencyNumber(options.number)) {
        if (DEBUG) {
          
          debug(options.number + " is not a valid emergency number.");
        }
        return;
      }
      if (RILQUIRKS_REQUEST_USE_DIAL_EMERGENCY_CALL) {
        dial_request_type = REQUEST_DIAL_EMERGENCY_CALL;
      }
    } else {
      if (this._isEmergencyNumber(options.number) &&
          RILQUIRKS_REQUEST_USE_DIAL_EMERGENCY_CALL) {
        dial_request_type = REQUEST_DIAL_EMERGENCY_CALL;
      }
    }

    let token = Buf.newParcel(dial_request_type);
    Buf.writeString(options.number);
    Buf.writeUint32(options.clirMode || 0);
    Buf.writeUint32(options.uusInfo || 0);
    
    
    Buf.writeUint32(0);
    Buf.sendParcel();
  },

  





  hangUp: function hangUp(options) {
    let call = this.currentCalls[options.callIndex];
    if (call && call.state != CALL_STATE_HOLDING) {
      Buf.simpleRequest(REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND);
    }
  },

  





  setMute: function setMute(mute) {
    Buf.newParcel(REQUEST_SET_MUTE);
    Buf.writeUint32(1);
    Buf.writeUint32(mute ? 1 : 0);
    Buf.sendParcel();
  },

  





  answerCall: function answerCall(options) {
    
    
    
    
    let call = this.currentCalls[options.callIndex];
    if (!call) {
      return;
    }
    
    switch (call.state) {
      case CALL_STATE_INCOMING:
        Buf.simpleRequest(REQUEST_ANSWER);
        break;
      case CALL_STATE_WAITING:
        
        Buf.simpleRequest(REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE);
        break;
    }
  },

  





  rejectCall: function rejectCall(options) {
    
    
    
    
    let call = this.currentCalls[options.callIndex];
    if (!call) {
      return;
    }
    
    switch (call.state) {
      case CALL_STATE_INCOMING:
        Buf.simpleRequest(REQUEST_UDUB);
        break;
      case CALL_STATE_WAITING:
        
        Buf.simpleRequest(REQUEST_HANGUP_WAITING_OR_BACKGROUND);
        break;
    }
  },
  
  holdCall: function holdCall(options) {
    let call = this.currentCalls[options.callIndex];
    if (call && call.state == CALL_STATE_ACTIVE) {
      Buf.simpleRequest(REQUEST_SWITCH_HOLDING_AND_ACTIVE);
    }
  },

  resumeCall: function resumeCall(options) {
    let call = this.currentCalls[options.callIndex];
    if (call && call.state == CALL_STATE_HOLDING) {
      Buf.simpleRequest(REQUEST_SWITCH_HOLDING_AND_ACTIVE);
    }
  },

  













  sendSMS: function sendSMS(options) {
    
    if (!this.SMSC) {
      
      
      this.getSMSCAddress(options);
      return;
    }
    
    
    
    options.SMSC = this.SMSC;

    

    if (!options.retryCount) {
      options.retryCount = 0;
    }

    if (options.segmentMaxSeq > 1) {
      if (!options.segmentSeq) {
        
        options.segmentSeq = 1;
        options.body = options.segments[0].body;
        options.encodedBodyLength = options.segments[0].encodedBodyLength;
      }
    } else {
      options.body = options.fullBody;
      options.encodedBodyLength = options.encodedFullBodyLength;
    }

    Buf.newParcel(REQUEST_SEND_SMS, options);
    Buf.writeUint32(2);
    Buf.writeString(options.SMSC);
    GsmPDUHelper.writeMessage(options);
    Buf.sendParcel();
  },

  







  acknowledgeSMS: function acknowledgeSMS(success, cause) {
    let token = Buf.newParcel(REQUEST_SMS_ACKNOWLEDGE);
    Buf.writeUint32(2);
    Buf.writeUint32(success ? 1 : 0);
    Buf.writeUint32(cause);
    Buf.sendParcel();
  },

  





  startTone: function startTone(options) {
    Buf.newParcel(REQUEST_DTMF_START);
    Buf.writeString(options.dtmfChar);
    Buf.sendParcel();
  },

  stopTone: function stopTone() {
    Buf.simpleRequest(REQUEST_DTMF_STOP);
  },

  





  sendTone: function sendTone(options) {
    Buf.newParcel(REQUEST_DTMF);
    Buf.writeString(options.dtmfChar);
    Buf.sendParcel();
  },

  





  getSMSCAddress: function getSMSCAddress(pendingSMS) {
    Buf.simpleRequest(REQUEST_GET_SMSC_ADDRESS, pendingSMS);
  },

  





  setSMSCAddress: function setSMSCAddress(options) {
    Buf.newParcel(REQUEST_SET_SMSC_ADDRESS);
    Buf.writeString(options.SMSC);
    Buf.sendParcel();
  },

  





















  setupDataCall: function setupDataCall(options) {
    let token = Buf.newParcel(REQUEST_SETUP_DATA_CALL);
    Buf.writeUint32(7);
    Buf.writeString(options.radioTech.toString());
    Buf.writeString(DATACALL_PROFILE_DEFAULT.toString());
    Buf.writeString(options.apn);
    Buf.writeString(options.user);
    Buf.writeString(options.passwd);
    Buf.writeString(options.chappap.toString());
    Buf.writeString(options.pdptype);
    Buf.sendParcel();
    return token;
  },

  







  deactivateDataCall: function deactivateDataCall(options) {
    let datacall = this.currentDataCalls[options.cid];
    if (!datacall) {
      return;
    }

    let token = Buf.newParcel(REQUEST_DEACTIVATE_DATA_CALL);
    Buf.writeUint32(2);
    Buf.writeString(options.cid);
    Buf.writeString(options.reason || DATACALL_DEACTIVATE_NO_REASON);
    Buf.sendParcel();

    datacall.state = GECKO_NETWORK_STATE_DISCONNECTING;
    this.sendDOMMessage({type: "datacallstatechange",
                         datacall: datacall});
  },

  


  getDataCallList: function getDataCallList() {
    Buf.simpleRequest(REQUEST_DATA_CALL_LIST);
  },

  


  getFailCauseCode: function getFailCauseCode() {
    Buf.simpleRequest(REQUEST_LAST_CALL_FAIL_CAUSE);
  },

  





   _isEmergencyNumber: function _isEmergencyNumber(number) {
     
     let numbers = libcutils.property_get("ril.ecclist");
     if (!numbers) {
       
       numbers = libcutils.property_get("ro.ril.ecclist");
     }

     if (numbers) {
       numbers = numbers.split(",");
     } else {
       
       numbers = DEFAULT_EMERGENCY_NUMBERS;
     }

     return numbers.indexOf(number) != -1;
   },

  


  _processICCStatus: function _processICCStatus(iccStatus) {
    this.iccStatus = iccStatus;

    if ((!iccStatus) || (iccStatus.cardState == CARD_STATE_ABSENT)) {
      if (DEBUG) debug("ICC absent");
      if (this.cardState == GECKO_CARDSTATE_ABSENT) {
        this.operator = null;
        return;
      }
      this.cardState = GECKO_CARDSTATE_ABSENT;
      this.sendDOMMessage({type: "cardstatechange",
                           cardState: this.cardState});
      return;
    }

    let app = iccStatus.apps[iccStatus.gsmUmtsSubscriptionAppIndex];
    if (!app) {
      if (DEBUG) {
        debug("Subscription application is not present in iccStatus.");
      }
      if (this.cardState == GECKO_CARDSTATE_ABSENT) {
        return;
      }
      this.cardState = GECKO_CARDSTATE_ABSENT;
      this.operator = null;
      this.sendDOMMessage({type: "cardstatechange",
                           cardState: this.cardState});
      return;
    }

    let newCardState;
    switch (app.app_state) {
      case CARD_APPSTATE_PIN:
        newCardState = GECKO_CARDSTATE_PIN_REQUIRED;
        break;
      case CARD_APPSTATE_PUK:
        newCardState = GECKO_CARDSTATE_PUK_REQUIRED;
        break;
      case CARD_APPSTATE_SUBSCRIPTION_PERSO:
        newCardState = GECKO_CARDSTATE_NETWORK_LOCKED;
        break;
      case CARD_APPSTATE_READY:
        this.requestNetworkInfo();
        this.getSignalStrength();
        this.fetchICCRecords();
        newCardState = GECKO_CARDSTATE_READY;
        break;
      case CARD_APPSTATE_UNKNOWN:
      case CARD_APPSTATE_DETECTED:
      default:
        newCardState = GECKO_CARDSTATE_NOT_READY;
    }

    if (this.cardState == newCardState) {
      return;
    }
    this.cardState = newCardState;
    this.sendDOMMessage({type: "cardstatechange",
                         cardState: this.cardState});
  },

  


  _processICCIOGetResponse: function _processICCIOGetResponse(options) {
    let length = Buf.readUint32();

    

    
    Buf.seekIncoming(2 * PDU_HEX_OCTET_SIZE);

    
    let fileSize = (GsmPDUHelper.readHexOctet() << 8) |
                    GsmPDUHelper.readHexOctet();

    
    let fileId = (GsmPDUHelper.readHexOctet() << 8) |
                  GsmPDUHelper.readHexOctet();
    if (fileId != options.fileId) {
      if (DEBUG) {
        debug("Expected file ID " + options.fileId + " but read " + fileId);
      }
      return;
    }

    
    let fileType = GsmPDUHelper.readHexOctet();
    if (fileType != TYPE_EF) {
      if (DEBUG) {
        debug("Unexpected file type " + fileType);
      }
      return;
    }

    
    
    
    
    Buf.seekIncoming(((RESPONSE_DATA_STRUCTURE - RESPONSE_DATA_FILE_TYPE - 1) *
        PDU_HEX_OCTET_SIZE));

    
    let efType = GsmPDUHelper.readHexOctet();
    if (efType != options.type) {
      if (DEBUG) {
        debug("Expected EF type " + options.type + " but read " + efType);
      }
      return;
    }

    
    let recordSize = GsmPDUHelper.readHexOctet();

    let delimiter = Buf.readUint16();
    if (!(length & 1)) {
      delimiter |= Buf.readUint16();
    }
    if (DEBUG) {
      if (delimiter != 0) {
        debug("Something's wrong, found string delimiter: " + delimiter);
      }
    }

    switch (options.type) {
      case EF_TYPE_LINEAR_FIXED:
        
        options.command = ICC_COMMAND_READ_RECORD;
        options.p1 = 1; 
        options.p2 = READ_RECORD_ABSOLUTE_MODE;
        options.p3 = recordSize;
        this.iccIO(options);
        break;
      case EF_TYPE_TRANSPARENT:
        
        options.command = ICC_COMMAND_READ_BINARY;
        options.p3 = fileSize;
        this.iccIO(options);
        break;
    }
  },

  


  _processICCIOReadRecord: function _processICCIOReadRecord(options) {
    if (options.callback) {
      options.callback.call(this);
    }
  },

  


  _processICCIOReadBinary: function _processICCIOReadBinary(options) {
    if (options.callback) {
      options.callback.call(this);
    }
  },

  


  _processICCIO: function _processICCIO(options) {
    switch (options.command) {
      case ICC_COMMAND_GET_RESPONSE:
        this._processICCIOGetResponse(options);
        break;

      case ICC_COMMAND_READ_RECORD:
        this._processICCIOReadRecord(options);
        break;

      case ICC_COMMAND_READ_BINARY:
        this._processICCIOReadBinary(options);
        break;
    } 
  },

  _processVoiceRegistrationState: function _processVoiceRegistrationState(state) {
    this.initRILQuirks();

    let rs = this.voiceRegistrationState;
    let stateChanged = false;

    let regState = RIL.parseInt(state[0], NETWORK_CREG_STATE_UNKNOWN);
    if (rs.regState != regState) {
      rs.regState = regState;
      if (RILQUIRKS_MODEM_DEFAULTS_TO_EMERGENCY_MODE) {
        rs.emergencyCallsOnly =
          (regState != NETWORK_CREG_STATE_REGISTERED_HOME) &&
          (regState != NETWORK_CREG_STATE_REGISTERED_ROAMING);
      } else {
        rs.emergencyCallsOnly =
          (regState >= NETWORK_CREG_STATE_NOT_SEARCHING_EMERGENCY_CALLS) &&
          (regState <= NETWORK_CREG_STATE_UNKNOWN_EMERGENCY_CALLS);
      }
      stateChanged = true;
      if (regState == NETWORK_CREG_STATE_REGISTERED_HOME ||
          regState == NETWORK_CREG_STATE_REGISTERED_ROAMING) {
        RIL.getSMSCAddress();
      }
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
      rs.type = "voiceregistrationstatechange";
      this.sendDOMMessage(rs);
    }
  },

  _processDataRegistrationState: function _processDataRegistrationState(state) {
    let rs = this.dataRegistrationState;
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
      rs.type = "dataregistrationstatechange";
      this.sendDOMMessage(rs);
    }
  },

  


  _processCalls: function _processCalls(newCalls) {
    
    
    
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

  _handleChangedCallState: function _handleChangedCallState(changedCall) {
    let message = {type: "callStateChange",
                   call: changedCall};
    this.sendDOMMessage(message);
  },

  _handleDisconnectedCall: function _handleDisconnectedCall(disconnectedCall) {
    let message = {type: "callDisconnected",
                   call: disconnectedCall};
    this.sendDOMMessage(message);
  },

  _processDataCallList: function _processDataCallList(datacalls) {
    for each (let currentDataCall in this.currentDataCalls) {
      let updatedDataCall;
      if (datacalls) {
        updatedDataCall = datacalls[currentDataCall.cid];
        delete datacalls[currentDataCall.cid];
      }

      if (!updatedDataCall) {
        delete this.currentDataCalls[currentDataCall.callIndex];
        currentDataCall.state = GECKO_NETWORK_STATE_DISCONNECTED;
        this.sendDOMMessage({type: "datacallstatechange",
                             datacall: currentDataCall});
        continue;
      }

      this._setDataCallGeckoState(updatedDataCall);
      if (updatedDataCall.state != currentDataCall.state) {
        currentDataCall.status = updatedDataCall.status;
        currentDataCall.active = updatedDataCall.active;
        currentDataCall.state = updatedDataCall.state;
        this.sendDOMMessage({type: "datacallstatechange",
                             datacall: currentDataCall});
      }
    }

    for each (let newDataCall in datacalls) {
      this.currentDataCalls[newDataCall.cid] = newDataCall;
      this._setDataCallGeckoState(newDataCall);
      this.sendDOMMessage({type: "datacallstatechange",
                           datacall: newDataCall});
    }
  },

  _setDataCallGeckoState: function _setDataCallGeckoState(datacall) {
    switch (datacall.active) {
      case DATACALL_INACTIVE:
        datacall.state = GECKO_NETWORK_STATE_DISCONNECTED;
        break;
      case DATACALL_ACTIVE_DOWN:
        datacall.state = GECKO_NETWORK_STATE_SUSPENDED;
        if (RILQUIRKS_DATACALLSTATE_DOWN_IS_UP) {
          datacall.state = GECKO_NETWORK_STATE_CONNECTED;
        }
        break;
      case DATACALL_ACTIVE_UP:
        datacall.state = GECKO_NETWORK_STATE_CONNECTED;
        break;
    }
  },

  







  _processReceivedSms: function _processReceivedSms(length) {
    if (!length) {
      if (DEBUG) debug("Received empty SMS!");
      return null;
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

    return message;
  },

  







  _processSmsDeliver: function _processSmsDeliver(length) {
    let message = this._processReceivedSms(length);
    if (!message) {
      return PDU_FCS_UNSPECIFIED;
    }

    if (message.epid == PDU_PID_SHORT_MESSAGE_TYPE_0) {
      
      
      
      return PDU_FCS_OK;
    }

    if (message.header && (message.header.segmentMaxSeq > 1)) {
      message = this._processReceivedSmsSegment(message);
    } else {
      if (message.encoding == PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
        message.fullData = message.data;
        delete message.data;
      } else {
        message.fullBody = message.body;
        delete message.body;
      }
    }

    if (message) {
      message.type = "sms-received";
      this.sendDOMMessage(message);
    }

    return PDU_FCS_OK;
  },

  







  _processSmsStatusReport: function _processSmsStatusReport(length) {
    let message = this._processReceivedSms(length);
    if (!message) {
      return PDU_FCS_UNSPECIFIED;
    }

    let options = this._pendingSentSmsMap[message.messageRef];
    if (!options) {
      return PDU_FCS_OK;
    }

    let status = message.status;

    
    
    if ((status >= 0x80)
        || ((status >= PDU_ST_0_RESERVED_BEGIN)
            && (status < PDU_ST_0_SC_SPECIFIC_BEGIN))
        || ((status >= PDU_ST_1_RESERVED_BEGIN)
            && (status < PDU_ST_1_SC_SPECIFIC_BEGIN))
        || ((status >= PDU_ST_2_RESERVED_BEGIN)
            && (status < PDU_ST_2_SC_SPECIFIC_BEGIN))
        || ((status >= PDU_ST_3_RESERVED_BEGIN)
            && (status < PDU_ST_3_SC_SPECIFIC_BEGIN))
        ) {
      status = PDU_ST_3_SERVICE_REJECTED;
    }

    
    if ((status >>> 5) == 0x01) {
      return PDU_FCS_OK;
    }

    delete this._pendingSentSmsMap[message.messageRef];

    if ((status >>> 5) != 0x00) {
      
      
      return PDU_FCS_OK;
    }

    if ((options.segmentMaxSeq > 1)
        && (options.segmentSeq < options.segmentMaxSeq)) {
      
      this._processSentSmsSegment(options);
    } else {
      
      this.sendDOMMessage({
        type: "sms-delivered",
        envelopeId: options.envelopeId,
      });
    }

    return PDU_FCS_OK;
  },

  





  _processReceivedSmsSegment: function _processReceivedSmsSegment(original) {
    let hash = original.sender + ":" + original.header.segmentRef;
    let seq = original.header.segmentSeq;

    let options = this._receivedSmsSegmentsMap[hash];
    if (!options) {
      options = original;
      this._receivedSmsSegmentsMap[hash] = options;

      options.segmentMaxSeq = original.header.segmentMaxSeq;
      options.receivedSegments = 0;
      options.segments = [];
    } else if (options.segments[seq]) {
      
      if (DEBUG) {
        debug("Got duplicated segment no." + seq + " of a multipart SMS: "
              + JSON.stringify(original));
      }
      return null;
    }

    if (options.encoding == PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      options.segments[seq] = original.data;
      delete original.data;
    } else {
      options.segments[seq] = original.body;
      delete original.body;
    }
    options.receivedSegments++;
    if (options.receivedSegments < options.segmentMaxSeq) {
      if (DEBUG) {
        debug("Got segment no." + seq + " of a multipart SMS: "
              + JSON.stringify(options));
      }
      return null;
    }

    
    delete this._receivedSmsSegmentsMap[hash];

    
    if (options.encoding == PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      
      
      let fullDataLen = 0;
      for (let i = 1; i <= options.segmentMaxSeq; i++) {
        fullDataLen += options.segments[i].length;
      }

      options.fullData = new Uint8Array(fullDataLen);
      for (let d= 0, i = 1; i <= options.segmentMaxSeq; i++) {
        let data = options.segments[i];
        for (let j = 0; j < data.length; j++) {
          options.fullData[d++] = data[j];
        }
      }
    } else {
      options.fullBody = options.segments.join("");
    }

    if (DEBUG) {
      debug("Got full multipart SMS: " + JSON.stringify(options));
    }

    return options;
  },

  


  _processSentSmsSegment: function _processSentSmsSegment(options) {
    
    let next = options.segmentSeq;
    options.body = options.segments[next].body;
    options.encodedBodyLength = options.segments[next].encodedBodyLength;
    options.segmentSeq = next + 1;

    this.sendSMS(options);
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

  


  sendDOMMessage: function sendDOMMessage(message) {
    postMessage(message, "*");
  },

  





  handleParcel: function handleParcel(request_type, length, options) {
    let method = this[request_type];
    if (typeof method == "function") {
      if (DEBUG) debug("Handling parcel as " + method.name);
      method.call(this, length, options);
    }
  }
};

RIL[REQUEST_GET_SIM_STATUS] = function REQUEST_GET_SIM_STATUS(length, options) {
  if (options.rilRequestError) {
    return;
  }

  let iccStatus = {};
  iccStatus.cardState = Buf.readUint32(); 
  iccStatus.universalPINState = Buf.readUint32(); 
  iccStatus.gsmUmtsSubscriptionAppIndex = Buf.readUint32();
  iccStatus.cdmaSubscriptionAppIndex = Buf.readUint32();
  if (!RILQUIRKS_V5_LEGACY) {
    iccStatus.imsSubscriptionAppIndex = Buf.readUint32();
  }

  let apps_length = Buf.readUint32();
  if (apps_length > CARD_MAX_APPS) {
    apps_length = CARD_MAX_APPS;
  }

  iccStatus.apps = [];
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

  if (DEBUG) debug("iccStatus: " + JSON.stringify(iccStatus));
  this._processICCStatus(iccStatus);
};
RIL[REQUEST_ENTER_SIM_PIN] = function REQUEST_ENTER_SIM_PIN(length, options) {
  if (options.rilRequestError) {
    return;
  }

  let response = Buf.readUint32List();
  if (DEBUG) debug("REQUEST_ENTER_SIM_PIN returned " + response);
};
RIL[REQUEST_ENTER_SIM_PUK] = function REQUEST_ENTER_SIM_PUK(length, options) {
  if (options.rilRequestError) {
    return;
  }

  let response = Buf.readUint32List();
  if (DEBUG) debug("REQUEST_ENTER_SIM_PUK returned " + response);
};
RIL[REQUEST_ENTER_SIM_PIN2] = null;
RIL[REQUEST_ENTER_SIM_PUK2] = null;
RIL[REQUEST_CHANGE_SIM_PIN] = null;
RIL[REQUEST_CHANGE_SIM_PIN2] = null;
RIL[REQUEST_ENTER_NETWORK_DEPERSONALIZATION] = null;
RIL[REQUEST_GET_CURRENT_CALLS] = function REQUEST_GET_CURRENT_CALLS(length, options) {
  if (options.rilRequestError) {
    return;
  }

  this.initRILQuirks();

  let calls_length = 0;
  
  
  if (length) {
    calls_length = Buf.readUint32();
  }
  if (!calls_length) {
    this._processCalls(null);
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
  this._processCalls(calls);
};
RIL[REQUEST_DIAL] = null;
RIL[REQUEST_GET_IMSI] = function REQUEST_GET_IMSI(length, options) {
  if (options.rilRequestError) {
    return;
  }

  this.iccInfo.IMSI = Buf.readString();
};
RIL[REQUEST_HANGUP] = function REQUEST_HANGUP(length, options) {
  if (options.rilRequestError) {
    return;
  }

  this.getCurrentCalls();
}; 
RIL[REQUEST_HANGUP_WAITING_OR_BACKGROUND] = function REQUEST_HANGUP_WAITING_OR_BACKGROUND(length, options) {
  if (options.rilRequestError) {
    return;
  }
  
  this.getCurrentCalls();
};
RIL[REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND] = function REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND(length, options) {
  if (options.rilRequestError) {
    return;
  }

  this.getCurrentCalls();
};
RIL[REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE] = function REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE(length, options) {
  if (options.rilRequestError) {
    return;
  }

  this.getCurrentCalls();
};
RIL[REQUEST_SWITCH_HOLDING_AND_ACTIVE] = function REQUEST_SWITCH_HOLDING_AND_ACTIVE(length, options) {
  if (options.rilRequestError) {
    return;
  }

  
  
  
  this.getCurrentCalls();
};
RIL[REQUEST_CONFERENCE] = null;
RIL[REQUEST_UDUB] = null;
RIL[REQUEST_LAST_CALL_FAIL_CAUSE] = null;
RIL[REQUEST_SIGNAL_STRENGTH] = function REQUEST_SIGNAL_STRENGTH(length, options) {
  if (options.rilRequestError) {
    return;
  }

  let obj = {};

  
  
  let gsmSignalStrength = Buf.readUint32();
  obj.gsmSignalStrength = gsmSignalStrength & 0xff;
  
  obj.gsmBitErrorRate = Buf.readUint32();

  obj.gsmDBM = null;
  obj.gsmRelative = null;
  if (obj.gsmSignalStrength >= 0 && obj.gsmSignalStrength <= 31) {
    obj.gsmDBM = -113 + obj.gsmSignalStrength * 2;
    obj.gsmRelative = Math.floor(obj.gsmSignalStrength * 100 / 31);
  }

  
  
  
  
  if (obj.gsmSignalStrength == 99) {
    obj.gsmRelative = (gsmSignalStrength >> 8) * 25;
  }

  
  obj.cdmaDBM = Buf.readUint32();
  
  obj.cdmaECIO = Buf.readUint32();
  

  
  obj.evdoDBM = Buf.readUint32();
  
  obj.evdoECIO = Buf.readUint32();
  
  obj.evdoSNR = Buf.readUint32();

  
  if (!RILQUIRKS_V5_LEGACY) {
    
    obj.lteSignalStrength = Buf.readUint32();
    
    
    obj.lteRSRP = Buf.readUint32();
    
    
    obj.lteRSRQ = Buf.readUint32();
    
    
    obj.lteRSSNR = Buf.readUint32();
    
    obj.lteCQI = Buf.readUint32();
  }

  if (DEBUG) debug("Signal strength " + JSON.stringify(obj));
  obj.type = "signalstrengthchange";
  this.sendDOMMessage(obj);
};
RIL[REQUEST_VOICE_REGISTRATION_STATE] = function REQUEST_VOICE_REGISTRATION_STATE(length, options) {
  if (options.rilRequestError) {
    return;
  }

  let state = Buf.readStringList();
  if (DEBUG) debug("voice registration state: " + state);
  this._processVoiceRegistrationState(state);
};
RIL[REQUEST_DATA_REGISTRATION_STATE] = function REQUEST_DATA_REGISTRATION_STATE(length, options) {
  if (options.rilRequestError) {
    return;
  }

  let state = Buf.readStringList();
  this._processDataRegistrationState(state);
};
RIL[REQUEST_OPERATOR] = function REQUEST_OPERATOR(length, options) {
  if (options.rilRequestError) {
    return;
  }

  let operator = Buf.readStringList();
  if (DEBUG) debug("Operator data: " + operator);
  if (operator.length < 3) {
    if (DEBUG) debug("Expected at least 3 strings for operator.");
  }
  if (!this.operator ||
      this.operator.alphaLong  != operator[0] ||
      this.operator.alphaShort != operator[1] ||
      this.operator.numeric    != operator[2]) {
    this.operator = {type: "operatorchange",
                     alphaLong:  operator[0],
                     alphaShort: operator[1],
                     numeric:    operator[2]};
    this.sendDOMMessage(this.operator);
  }
};
RIL[REQUEST_RADIO_POWER] = null;
RIL[REQUEST_DTMF] = null;
RIL[REQUEST_SEND_SMS] = function REQUEST_SEND_SMS(length, options) {
  if (options.rilRequestError) {
    switch (options.rilRequestError) {
      case ERROR_SMS_SEND_FAIL_RETRY:
        if (options.retryCount < SMS_RETRY_MAX) {
          options.retryCount++;
          
          this.sendSMS(options);
          break;
        }

        
      default:
        this.sendDOMMessage({
          type: "sms-send-failed",
          envelopeId: options.envelopeId,
          error: options.rilRequestError,
        });
        break;
    }
    return;
  }

  options.messageRef = Buf.readUint32();
  options.ackPDU = Buf.readString();
  options.errorCode = Buf.readUint32();

  if (options.requestStatusReport) {
    this._pendingSentSmsMap[options.messageRef] = options;
  }

  if ((options.segmentMaxSeq > 1)
      && (options.segmentSeq < options.segmentMaxSeq)) {
    
    if (!options.requestStatusReport) {
      
      this._processSentSmsSegment(options);
    }
  } else {
    
    this.sendDOMMessage({
      type: "sms-sent",
      envelopeId: options.envelopeId,
    });
  }
};
RIL[REQUEST_SEND_SMS_EXPECT_MORE] = null;

RIL.readSetupDataCall_v5 = function readSetupDataCall_v5(options) {
  if (!options) {
    options = {};
  }
  let [cid, ifname, ipaddr, dns, gw] = Buf.readStringList();
  options.cid = cid;
  options.ifname = ifname;
  options.ipaddr = ipaddr;
  options.dns = dns;
  options.gw = gw;
  options.active = DATACALL_ACTIVE_UNKNOWN;
  options.state = GECKO_NETWORK_STATE_CONNECTING;
  return options;
};

RIL[REQUEST_SETUP_DATA_CALL] = function REQUEST_SETUP_DATA_CALL(length, options) {
  if (options.rilRequestError) {
    return;
  }

  if (RILQUIRKS_V5_LEGACY) {
    this.readSetupDataCall_v5(options);
    this.currentDataCalls[options.cid] = options;
    this.sendDOMMessage({type: "datacallstatechange",
                         datacall: options});
    
    
    this.getDataCallList();
    return;
  }
  this[REQUEST_DATA_CALL_LIST](length, options);
};
RIL[REQUEST_SIM_IO] = function REQUEST_SIM_IO(length, options) {
  if (options.rilRequestError) {
    return;
  }

  let sw1 = Buf.readUint32();
  let sw2 = Buf.readUint32();
  if (sw1 != ICC_STATUS_NORMAL_ENDING) {
    
    
    if (DEBUG) {
      debug("ICC I/O Error EF id = " + options.fileId.toString(16) +
            " command = " + options.command.toString(16) +
            "(" + sw1.toString(16) + "/" + sw2.toString(16) + ")");
    }
    return;
  }
  this._processICCIO(options);
};
RIL[REQUEST_SEND_USSD] = null;
RIL[REQUEST_CANCEL_USSD] = null;
RIL[REQUEST_GET_CLIR] = null;
RIL[REQUEST_SET_CLIR] = null;
RIL[REQUEST_QUERY_CALL_FORWARD_STATUS] = null;
RIL[REQUEST_SET_CALL_FORWARD] = null;
RIL[REQUEST_QUERY_CALL_WAITING] = null;
RIL[REQUEST_SET_CALL_WAITING] = null;
RIL[REQUEST_SMS_ACKNOWLEDGE] = null;
RIL[REQUEST_GET_IMEI] = function REQUEST_GET_IMEI(length, options) {
  if (options.rilRequestError) {
    return;
  }

  this.IMEI = Buf.readString();
};
RIL[REQUEST_GET_IMEISV] = function REQUEST_GET_IMEISV(length, options) {
  if (options.rilRequestError) {
    return;
  }

  this.IMEISV = Buf.readString();
};
RIL[REQUEST_ANSWER] = null;
RIL[REQUEST_DEACTIVATE_DATA_CALL] = function REQUEST_DEACTIVATE_DATA_CALL(length, options) {
  if (options.rilRequestError) {
    return;
  }

  let datacall = this.currentDataCalls[options.cid];
  delete this.currentDataCalls[options.cid];
  datacall.state = GECKO_NETWORK_STATE_DISCONNECTED;
  this.sendDOMMessage({type: "datacallstatechange",
                       datacall: datacall});
};
RIL[REQUEST_QUERY_FACILITY_LOCK] = null;
RIL[REQUEST_SET_FACILITY_LOCK] = null;
RIL[REQUEST_CHANGE_BARRING_PASSWORD] = null;
RIL[REQUEST_QUERY_NETWORK_SELECTION_MODE] = function REQUEST_QUERY_NETWORK_SELECTION_MODE(length, options) {
  if (options.rilRequestError) {
    return;
  }

  let mode = Buf.readUint32List();
  this.networkSelectionMode = mode[0];
};
RIL[REQUEST_SET_NETWORK_SELECTION_AUTOMATIC] = null;
RIL[REQUEST_SET_NETWORK_SELECTION_MANUAL] = null;
RIL[REQUEST_QUERY_AVAILABLE_NETWORKS] = null;
RIL[REQUEST_DTMF_START] = null;
RIL[REQUEST_DTMF_STOP] = null;
RIL[REQUEST_BASEBAND_VERSION] = function REQUEST_BASEBAND_VERSION(length, options) {
  if (options.rilRequestError) {
    return;
  }

  this.basebandVersion = Buf.readString();
  if (DEBUG) debug("Baseband version: " + this.basebandVersion);
};
RIL[REQUEST_SEPARATE_CONNECTION] = null;
RIL[REQUEST_SET_MUTE] = null;
RIL[REQUEST_GET_MUTE] = null;
RIL[REQUEST_QUERY_CLIP] = null;
RIL[REQUEST_LAST_DATA_CALL_FAIL_CAUSE] = null;

RIL.readDataCall_v5 = function readDataCall_v5() {
  return {
    cid: Buf.readUint32().toString(),
    active: Buf.readUint32(), 
    type: Buf.readString(),
    apn: Buf.readString(),
    address: Buf.readString()
  };
};

RIL.readDataCall_v6 = function readDataCall_v6(obj) {
  if (!obj) {
    obj = {};
  }
  obj.status = Buf.readUint32();  
  obj.suggestedRetryTime = Buf.readUint32();
  obj.cid = Buf.readUint32().toString();
  obj.active = Buf.readUint32();  
  obj.type = Buf.readString();
  obj.ifname = Buf.readString();
  obj.ipaddr = Buf.readString();
  obj.dns = Buf.readString();
  obj.gw = Buf.readString();
  if (obj.dns) {
    obj.dns = obj.dns.split(" ");
  }
  
  if (obj.ipaddr) {
    obj.ipaddr = obj.ipaddr.split(" ")[0];
  }
  if (obj.gw) {
    obj.gw = obj.gw.split(" ")[0];
  }
  return obj;
};

RIL[REQUEST_DATA_CALL_LIST] = function REQUEST_DATA_CALL_LIST(length, options) {
  if (options.rilRequestError) {
    return;
  }

  this.initRILQuirks();
  if (!length) {
    this._processDataCallList(null);
    return;
  }

  let version = 0;
  if (!RILQUIRKS_V5_LEGACY) {
    version = Buf.readUint32();
  }
  let num = num = Buf.readUint32();
  let datacalls = {};
  for (let i = 0; i < num; i++) {
    let datacall;
    if (version < 6) {
      datacall = this.readDataCall_v5();
    } else {
      datacall = this.readDataCall_v6();
    }
    datacalls[datacall.cid] = datacall;
  }

  this._processDataCallList(datacalls);
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
RIL[REQUEST_CDMA_SET_SUBSCRIPTION_SOURCE] = null;
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
RIL[REQUEST_GET_SMSC_ADDRESS] = function REQUEST_GET_SMSC_ADDRESS(length, options) {
  if (options.rilRequestError) {
    if (options.type == "sendSMS") {
      this.sendDOMMessage({
        type: "sms-send-failed",
        envelopeId: options.envelopeId,
        error: options.rilRequestError,
      });
    }
    return;
  }

  this.SMSC = Buf.readString();
  
  
  
  
  if (this.SMSC && options.body) {
    this.sendSMS(options);
  }
};
RIL[REQUEST_SET_SMSC_ADDRESS] = null;
RIL[REQUEST_REPORT_SMS_MEMORY_STATUS] = null;
RIL[REQUEST_REPORT_STK_SERVICE_IS_RUNNING] = null;
RIL[UNSOLICITED_RESPONSE_RADIO_STATE_CHANGED] = function UNSOLICITED_RESPONSE_RADIO_STATE_CHANGED() {
  let radioState = Buf.readUint32();

  let newState;
  if (radioState == RADIO_STATE_UNAVAILABLE) {
    newState = GECKO_RADIOSTATE_UNAVAILABLE;
  } else if (radioState == RADIO_STATE_OFF) {
    newState = GECKO_RADIOSTATE_OFF;
  } else {
    newState = GECKO_RADIOSTATE_READY;
  }

  if (DEBUG) {
    debug("Radio state changed from '" + this.radioState +
          "' to '" + newState + "'");
  }
  if (this.radioState == newState) {
    return;
  }

  
  let cdma = false;

  if (this.radioState == GECKO_RADIOSTATE_UNAVAILABLE &&
      newState != GECKO_RADIOSTATE_UNAVAILABLE) {
    
    if (cdma) {
      this.getDeviceIdentity();
    } else {
      this.getIMEI();
      this.getIMEISV();
    }
    this.getBasebandVersion();

    
    
    
    if (newState == GECKO_RADIOSTATE_OFF) {
      this.setRadioPower(true);
    }
  }

  this.radioState = newState;
  this.sendDOMMessage({
    type: "radiostatechange",
    radioState: newState
  });

  
  
  if (radioState == RADIO_STATE_UNAVAILABLE ||
      radioState == RADIO_STATE_OFF) {
    return;
  }
  this.getICCStatus();
};
RIL[UNSOLICITED_RESPONSE_CALL_STATE_CHANGED] = function UNSOLICITED_RESPONSE_CALL_STATE_CHANGED() {
  this.getCurrentCalls();
};
RIL[UNSOLICITED_RESPONSE_VOICE_NETWORK_STATE_CHANGED] = function UNSOLICITED_RESPONSE_VOICE_NETWORK_STATE_CHANGED() {
  if (DEBUG) debug("Network state changed, re-requesting phone state.");
  this.requestNetworkInfo();
};
RIL[UNSOLICITED_RESPONSE_NEW_SMS] = function UNSOLICITED_RESPONSE_NEW_SMS(length) {
  let result = this._processSmsDeliver(length);
  this.acknowledgeSMS(result == PDU_FCS_OK, result);
};
RIL[UNSOLICITED_RESPONSE_NEW_SMS_STATUS_REPORT] = function UNSOLICITED_RESPONSE_NEW_SMS_STATUS_REPORT(length) {
  let result = this._processSmsStatusReport(length);
  this.acknowledgeSMS(result == PDU_FCS_OK, result);
};
RIL[UNSOLICITED_RESPONSE_NEW_SMS_ON_SIM] = function UNSOLICITED_RESPONSE_NEW_SMS_ON_SIM(length) {
  let info = Buf.readUint32List();
  
};
RIL[UNSOLICITED_ON_USSD] = null;
RIL[UNSOLICITED_ON_USSD_REQUEST] = null;

RIL[UNSOLICITED_NITZ_TIME_RECEIVED] = function UNSOLICITED_NITZ_TIME_RECEIVED() {
  let dateString = Buf.readString();

  
  
  

  
  
  
  
  debug("DateTimeZone string " + dateString);

  let now = Date.now();
	
  let year = parseInt(dateString.substr(0, 2), 10);
  let month = parseInt(dateString.substr(3, 2), 10);
  let day = parseInt(dateString.substr(6, 2), 10);
  let hours = parseInt(dateString.substr(9, 2), 10);
  let minutes = parseInt(dateString.substr(12, 2), 10);
  let seconds = parseInt(dateString.substr(15, 2), 10);
  let tz = parseInt(dateString.substr(17, 3), 10); 
  let dst = parseInt(dateString.substr(21, 2), 10); 

  let timeInSeconds = Date.UTC(year + PDU_TIMESTAMP_YEAR_OFFSET, month - 1, day,
                               hours, minutes, seconds) / 1000;

  if (isNaN(timeInSeconds)) {
    if (DEBUG) debug("NITZ failed to convert date");
    return;
  }

  this.sendDOMMessage({type: "nitzTime",
                       networkTimeInSeconds: timeInSeconds,
                       networkTimeZoneInMinutes: tz * 15,
                       dstFlag: dst,
                       localTimeStampInMS: now});
};

RIL[UNSOLICITED_SIGNAL_STRENGTH] = function UNSOLICITED_SIGNAL_STRENGTH(length) {
  this[REQUEST_SIGNAL_STRENGTH](length, {rilRequestError: ERROR_SUCCESS});
};
RIL[UNSOLICITED_DATA_CALL_LIST_CHANGED] = function UNSOLICITED_DATA_CALL_LIST_CHANGED(length) {
  if (RILQUIRKS_V5_LEGACY) {
    this.getDataCallList();
    return;
  }
  this[REQUEST_GET_DATA_CALL_LIST](length, {rilRequestError: ERROR_SUCCESS});
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
  
  
};
RIL[UNSOLICITED_RESPONSE_SIM_STATUS_CHANGED] = function UNSOLICITED_RESPONSE_SIM_STATUS_CHANGED() {
  this.getICCStatus();
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
RIL[UNSOLICITED_RIL_CONNECTED] = function UNSOLICITED_RIL_CONNECTED(length) {
  
  
  if (!length) {
    this.initRILQuirks();
    return;
  }

  let version = Buf.readUint32List()[0];
  RILQUIRKS_V5_LEGACY = (version < 5);
  if (DEBUG) {
    debug("Detected RIL version " + version);
    debug("RILQUIRKS_V5_LEGACY is " + RILQUIRKS_V5_LEGACY);
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

  


  readHexOctetArray: function readHexOctetArray(length) {
    let array = new Uint8Array(length);
    for (let i = 0; i < length; i++) {
      array[i] = this.readHexOctet();
    }
    return array;
  },

  









  octetToBCD: function octetToBCD(octet) {
    return ((octet & 0xf0) <= 0x90) * ((octet >> 4) & 0x0f) +
           ((octet & 0x0f) <= 0x09) * (octet & 0x0f) * 10;
  },

  







  readSwappedNibbleBCD: function readSwappedNibbleBCD(length) {
    let number = 0;
    for (let i = 0; i < length; i++) {
      let octet = this.readHexOctet();
      
      if (octet == 0xff) {
        continue;
      }
      
      
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

  



 
  readStringAsBCD: function readStringAsBCD() {
    let length = Buf.readUint32();
    let bcd = this.readSwappedNibbleBCD(length / 2);
    let delimiter = Buf.readUint16();
    if (!(length & 1)) {
      delimiter |= Buf.readUint16();
    }
    if (DEBUG) {
      if (delimiter != 0) {
        debug("Something's wrong, found string delimiter: " + delimiter);
      }
    }
    return bcd;
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

  














  readSeptetsToString: function readSeptetsToString(length, paddingBits, langIndex, langShiftIndex) {
    let ret = "";
    let byteLength = Math.ceil((length * 7 + paddingBits) / 8);

    












    let data = 0;
    let dataBits = 0;
    if (paddingBits) {
      data = this.readHexOctet() >> paddingBits;
      dataBits = 8 - paddingBits;
      --byteLength;
    }

    let escapeFound = false;
    const langTable = PDU_NL_LOCKING_SHIFT_TABLES[langIndex];
    const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[langShiftIndex];
    do {
      
      let bytesToRead = Math.min(byteLength, dataBits ? 3 : 4);
      for (let i = 0; i < bytesToRead; i++) {
        data |= this.readHexOctet() << dataBits;
        dataBits += 8;
        --byteLength;
      }

      
      for (; dataBits >= 7; dataBits -= 7) {
        let septet = data & 0x7F;
        data >>>= 7;

        if (escapeFound) {
          escapeFound = false;
          if (septet == PDU_NL_EXTENDED_ESCAPE) {
            
            
            
            ret += " ";
          } else if (septet == PDU_NL_RESERVED_CONTROL) {
            
            
            
            ret += " ";
          } else {
            ret += langShiftTable[septet];
          }
        } else if (septet == PDU_NL_EXTENDED_ESCAPE) {
          escapeFound = true;

          
          --length;
        } else {
          ret += langTable[septet];
        }
      }
    } while (byteLength);

    if (ret.length != length) {
      















      ret = ret.slice(0, length);
    }
    return ret;
  },

  writeStringAsSeptets: function writeStringAsSeptets(message, paddingBits, langIndex, langShiftIndex) {
    const langTable = PDU_NL_LOCKING_SHIFT_TABLES[langIndex];
    const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[langShiftIndex];

    let dataBits = paddingBits;
    let data = 0;
    for (let i = 0; i < message.length; i++) {
      let septet = langTable.indexOf(message[i]);
      if (septet == PDU_NL_EXTENDED_ESCAPE) {
        continue;
      }

      if (septet >= 0) {
        data |= septet << dataBits;
        dataBits += 7;
      } else {
        septet = langShiftTable.indexOf(message[i]);
        if (septet == -1) {
          throw new Error(message[i] + " not in 7 bit alphabet "
                          + langIndex + ":" + langShiftIndex + "!");
        }

        if (septet == PDU_NL_RESERVED_CONTROL) {
          continue;
        }

        data |= PDU_NL_EXTENDED_ESCAPE << dataBits;
        dataBits += 7;
        data |= septet << dataBits;
        dataBits += 7;
      }

      for (; dataBits >= 8; dataBits -= 8) {
        this.writeHexOctet(data & 0xFF);
        data >>>= 8;
      }
    }

    if (dataBits != 0) {
      this.writeHexOctet(data & 0xFF);
    }
  },

  







  readUCS2String: function readUCS2String(numOctets) {
    let str = "";
    let length = numOctets / 2;
    for (let i = 0; i < length; ++i) {
      let code = (this.readHexOctet() << 8) | this.readHexOctet();
      str += String.fromCharCode(code);
    }

    if (DEBUG) debug("Read UCS2 string: " + str);

    return str;
  },

  





  writeUCS2String: function writeUCS2String(message) {
    for (let i = 0; i < message.length; ++i) {
      let code = message.charCodeAt(i);
      this.writeHexOctet((code >> 8) & 0xFF);
      this.writeHexOctet(code & 0xFF);
    }
  },

  












  readUserDataHeader: function readUserDataHeader() {
    let header = {
      length: 0,
      langIndex: PDU_NL_IDENTIFIER_DEFAULT,
      langShiftIndex: PDU_NL_IDENTIFIER_DEFAULT
    };

    header.length = this.readHexOctet();
    let dataAvailable = header.length;
    while (dataAvailable >= 2) {
      let id = this.readHexOctet();
      let length = this.readHexOctet();
      dataAvailable -= 2;

      switch (id) {
        case PDU_IEI_CONCATENATED_SHORT_MESSAGES_8BIT: {
          let ref = this.readHexOctet();
          let max = this.readHexOctet();
          let seq = this.readHexOctet();
          dataAvailable -= 3;
          if (max && seq && (seq <= max)) {
            header.segmentRef = ref;
            header.segmentMaxSeq = max;
            header.segmentSeq = seq;
          }
          break;
        }
        case PDU_IEI_APPLICATION_PORT_ADDREESING_SCHEME_8BIT: {
          let dstp = this.readHexOctet();
          let orip = this.readHexOctet();
          dataAvailable -= 2;
          if ((dstp < PDU_APA_RESERVED_8BIT_PORTS)
              || (orip < PDU_APA_RESERVED_8BIT_PORTS)) {
            
            
            
            break;
          }
          header.destinationPort = dstp;
          header.originatorPort = orip;
          break;
        }
        case PDU_IEI_APPLICATION_PORT_ADDREESING_SCHEME_16BIT: {
          let dstp = (this.readHexOctet() << 8) | this.readHexOctet();
          let orip = (this.readHexOctet() << 8) | this.readHexOctet();
          dataAvailable -= 4;
          
          
          
          if ((dstp < PDU_APA_VALID_16BIT_PORTS)
              && (orip < PDU_APA_VALID_16BIT_PORTS)) {
            header.destinationPort = dstp;
            header.originatorPort = orip;
          }
          break;
        }
        case PDU_IEI_CONCATENATED_SHORT_MESSAGES_16BIT: {
          let ref = (this.readHexOctet() << 8) | this.readHexOctet();
          let max = this.readHexOctet();
          let seq = this.readHexOctet();
          dataAvailable -= 4;
          if (max && seq && (seq <= max)) {
            header.segmentRef = ref;
            header.segmentMaxSeq = max;
            header.segmentSeq = seq;
          }
          break;
        }
        case PDU_IEI_NATIONAL_LANGUAGE_SINGLE_SHIFT:
          let langShiftIndex = this.readHexOctet();
          --dataAvailable;
          if (langShiftIndex < PDU_NL_SINGLE_SHIFT_TABLES.length) {
            header.langShiftIndex = langShiftIndex;
          }
          break;
        case PDU_IEI_NATIONAL_LANGUAGE_LOCKING_SHIFT:
          let langIndex = this.readHexOctet();
          --dataAvailable;
          if (langIndex < PDU_NL_LOCKING_SHIFT_TABLES.length) {
            header.langIndex = langIndex;
          }
          break;
        default:
          if (DEBUG) {
            debug("readUserDataHeader: unsupported IEI(" + id
                  + "), " + length + " bytes.");
          }

          
          if (length) {
            let octets;
            if (DEBUG) octets = new Uint8Array(length);

            for (let i = 0; i < length; i++) {
              let octet = this.readHexOctet();
              if (DEBUG) octets[i] = octet;
            }
            dataAvailable -= length;

            if (DEBUG) debug("readUserDataHeader: " + Array.slice(octets));
          }
          break;
      }
    }

    if (dataAvailable != 0) {
      throw new Error("Illegal user data header found!");
    }

    return header;
  },

  






  writeUserDataHeader: function writeUserDataHeader(options) {
    this.writeHexOctet(options.userDataHeaderLength);

    if (options.segmentMaxSeq > 1) {
      if (options.segmentRef16Bit) {
        this.writeHexOctet(PDU_IEI_CONCATENATED_SHORT_MESSAGES_16BIT);
        this.writeHexOctet(4);
        this.writeHexOctet((options.segmentRef >> 8) & 0xFF);
      } else {
        this.writeHexOctet(PDU_IEI_CONCATENATED_SHORT_MESSAGES_8BIT);
        this.writeHexOctet(3);
      }
      this.writeHexOctet(options.segmentRef & 0xFF);
      this.writeHexOctet(options.segmentMaxSeq & 0xFF);
      this.writeHexOctet(options.segmentSeq & 0xFF);
    }

    if (options.langIndex != PDU_NL_IDENTIFIER_DEFAULT) {
      this.writeHexOctet(PDU_IEI_NATIONAL_LANGUAGE_LOCKING_SHIFT);
      this.writeHexOctet(1);
      this.writeHexOctet(options.langIndex);
    }

    if (options.langShiftIndex != PDU_NL_IDENTIFIER_DEFAULT) {
      this.writeHexOctet(PDU_IEI_NATIONAL_LANGUAGE_SINGLE_SHIFT);
      this.writeHexOctet(1);
      this.writeHexOctet(options.langShiftIndex);
    }
  },

  




  readAddress: function readAddress(len) {
    
    if (!len || (len < 0)) {
      if (DEBUG) debug("PDU error: invalid sender address length: " + len);
      return null;
    }
    if (len % 2 == 1) {
      len += 1;
    }
    if (DEBUG) debug("PDU: Going to read address: " + len);

    
    let toa = this.readHexOctet();

    
    let addr = this.readSwappedNibbleBCD(len / 2).toString();
    if (addr.length <= 0) {
      if (DEBUG) debug("PDU error: no number provided");
      return null;
    }
    if ((toa >> 4) == (PDU_TOA_INTERNATIONAL >> 4)) {
      addr = '+' + addr;
    }

    return addr;
  },

  







  readProtocolIndicator: function readProtocolIndicator(msg) {
    
    
    msg.pid = this.readHexOctet();

    msg.epid = msg.pid;
    switch (msg.epid & 0xC0) {
      case 0x40:
        
        switch (msg.epid) {
          case PDU_PID_SHORT_MESSAGE_TYPE_0:
            return;
        }
        break;
    }
    msg.epid = PDU_PID_DEFAULT;
  },

  







  readDataCodingScheme: function readDataCodingScheme(msg) {
    let dcs = this.readHexOctet();

    
    let encoding = PDU_DCS_MSG_CODING_7BITS_ALPHABET;
    switch (dcs & 0xC0) {
      case 0x0:
        
        switch (dcs & 0x0C) {
          case 0x4:
            encoding = PDU_DCS_MSG_CODING_8BITS_ALPHABET;
            break;
          case 0x8:
            encoding = PDU_DCS_MSG_CODING_16BITS_ALPHABET;
            break;
        }
        break;
      case 0xC0:
        
        switch (dcs & 0x30) {
          case 0x20:
            encoding = PDU_DCS_MSG_CODING_16BITS_ALPHABET;
            break;
          case 0x30:
            if (dcs & 0x04) {
              encoding = PDU_DCS_MSG_CODING_8BITS_ALPHABET;
            }
            break;
        }
        break;
      default:
        
        break;
    }

    msg.dcs = dcs;
    msg.encoding = encoding;

    if (DEBUG) debug("PDU: message encoding is " + encoding + " bit.");
  },

  




  readTimestamp: function readTimestamp() {
    let year   = this.readSwappedNibbleBCD(1) + PDU_TIMESTAMP_YEAR_OFFSET;
    let month  = this.readSwappedNibbleBCD(1) - 1;
    let day    = this.readSwappedNibbleBCD(1);
    let hour   = this.readSwappedNibbleBCD(1);
    let minute = this.readSwappedNibbleBCD(1);
    let second = this.readSwappedNibbleBCD(1);
    let timestamp = Date.UTC(year, month, day, hour, minute, second);

    
    
    
    
    
    let tzOctet = this.readHexOctet();
    let tzOffset = this.octetToBCD(tzOctet & ~0x08) * 15 * 60 * 1000;
    tzOffset = (tzOctet & 0x08) ? -tzOffset : tzOffset;
    timestamp -= tzOffset;

    return timestamp;
  },

  








  readUserData: function readUserData(msg, length) {
    if (DEBUG) {
      debug("Reading " + length + " bytes of user data.");
    }

    let paddingBits = 0;
    if (msg.udhi) {
      msg.header = this.readUserDataHeader();

      if (msg.encoding == PDU_DCS_MSG_CODING_7BITS_ALPHABET) {
        let headerBits = (msg.header.length + 1) * 8;
        let headerSeptets = Math.ceil(headerBits / 7);

        length -= headerSeptets;
        paddingBits = headerSeptets * 7 - headerBits;
      } else {
        length -= (msg.header.length + 1);
      }
    }

    msg.body = null;
    msg.data = null;
    switch (msg.encoding) {
      case PDU_DCS_MSG_CODING_7BITS_ALPHABET:
        
        
        if (length > PDU_MAX_USER_DATA_7BIT) {
          if (DEBUG) debug("PDU error: user data is too long: " + length);
          break;
        }

        let langIndex = msg.udhi ? msg.header.langIndex : PDU_NL_IDENTIFIER_DEFAULT;
        let langShiftIndex = msg.udhi ? msg.header.langShiftIndex : PDU_NL_IDENTIFIER_DEFAULT;
        msg.body = this.readSeptetsToString(length, paddingBits, langIndex,
                                            langShiftIndex);
        break;
      case PDU_DCS_MSG_CODING_8BITS_ALPHABET:
        msg.data = this.readHexOctetArray(length);
        break;
      case PDU_DCS_MSG_CODING_16BITS_ALPHABET:
        msg.body = this.readUCS2String(length);
        break;
    }
  },

  





  readExtraParams: function readExtraParams(msg) {
    
    
    
    if (Buf.readAvailable <= 4) {
      return;
    }

    
    let pi;
    do {
      
      
      
      
      pi = this.readHexOctet();
    } while (pi & PDU_PI_EXTENSION);

    
    
    
    msg.dcs = 0;
    msg.encoding = PDU_DCS_MSG_CODING_7BITS_ALPHABET;

    
    if (pi & PDU_PI_PROTOCOL_IDENTIFIER) {
      this.readProtocolIndicator(msg);
    }
    
    if (pi & PDU_PI_DATA_CODING_SCHEME) {
      this.readDataCodingScheme(msg);
    }
    
    if (pi & PDU_PI_USER_DATA_LENGTH) {
      let userDataLength = this.readHexOctet();
      this.readUserData(msg, userDataLength);
    }
  },

  





  readMessage: function readMessage() {
    
    let msg = {
      
      
      
      
      SMSC:      null, 
      mti:       null, 
      udhi:      null, 
      sender:    null, 
      recipient: null, 
      pid:       null, 
      epid:      null, 
      dcs:       null, 
      encoding:  null, 
      body:      null, 
      data:      null, 
      timestamp: null, 
      status:    null, 
      scts:      null, 
      dt:        null, 
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
    
    msg.mti = firstOctet & 0x03;
    
    msg.udhi = firstOctet & PDU_UDHI;

    switch (msg.mti) {
      case PDU_MTI_SMS_RESERVED:
        
        
        
      case PDU_MTI_SMS_DELIVER:
        return this.readDeliverMessage(msg);
      case PDU_MTI_SMS_STATUS_REPORT:
        return this.readStatusReportMessage(msg);
      default:
        return null;
    }
  },

  





  readDeliverMessage: function readDeliverMessage(msg) {
    
    let senderAddressLength = this.readHexOctet();
    msg.sender = this.readAddress(senderAddressLength);
    
    this.readProtocolIndicator(msg);
    
    this.readDataCodingScheme(msg);
    
    msg.timestamp = this.readTimestamp();
    
    let userDataLength = this.readHexOctet();

    
    if (userDataLength > 0) {
      this.readUserData(msg, userDataLength);
    }

    return msg;
  },

  





  readStatusReportMessage: function readStatusReportMessage(msg) {
    
    msg.messageRef = this.readHexOctet();
    
    let recipientAddressLength = this.readHexOctet();
    msg.recipient = this.readAddress(recipientAddressLength);
    
    msg.scts = this.readTimestamp();
    
    msg.dt = this.readTimestamp();
    
    msg.status = this.readHexOctet();

    this.readExtraParams(msg);

    return msg;
  },

  


























  writeMessage: function writeMessage(options) {
    if (DEBUG) {
      debug("writeMessage: " + JSON.stringify(options));
    }
    let address = options.number;
    let body = options.body;
    let dcs = options.dcs;
    let userDataHeaderLength = options.userDataHeaderLength;
    let encodedBodyLength = options.encodedBodyLength;
    let langIndex = options.langIndex;
    let langShiftIndex = options.langShiftIndex;

    
    
    
    
    
    
    
    
    
    

    let addressFormat = PDU_TOA_ISDN; 
    if (address[0] == '+') {
      addressFormat = PDU_TOA_INTERNATIONAL | PDU_TOA_ISDN; 
      address = address.substring(1);
    }
    
    let validity = 0;

    let headerOctets = (userDataHeaderLength ? userDataHeaderLength + 1 : 0);
    let paddingBits;
    let userDataLengthInSeptets;
    let userDataLengthInOctets;
    if (dcs == PDU_DCS_MSG_CODING_7BITS_ALPHABET) {
      let headerSeptets = Math.ceil(headerOctets * 8 / 7);
      userDataLengthInSeptets = headerSeptets + encodedBodyLength;
      userDataLengthInOctets = Math.ceil(userDataLengthInSeptets * 7 / 8);
      paddingBits = headerSeptets * 7 - headerOctets * 8;
    } else {
      userDataLengthInOctets = headerOctets + encodedBodyLength;
      paddingBits = 0;
    }

    let pduOctetLength = 4 + 
                         Math.ceil(address.length / 2) +
                         3 + 
                         userDataLengthInOctets;
    if (validity) {
      
    }

    
    
    Buf.writeUint32(pduOctetLength * 2);

    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    let firstOctet = PDU_MTI_SMS_SUBMIT;

    
    if (options.requestStatusReport) {
      firstOctet |= PDU_SRI_SRR;
    }

    
    if (validity) {
      
    }
    
    if (headerOctets) {
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

    
    if (dcs == PDU_DCS_MSG_CODING_7BITS_ALPHABET) {
      this.writeHexOctet(userDataLengthInSeptets);
    } else {
      this.writeHexOctet(userDataLengthInOctets);
    }

    if (headerOctets) {
      this.writeUserDataHeader(options);
    }

    switch (dcs) {
      case PDU_DCS_MSG_CODING_7BITS_ALPHABET:
        this.writeStringAsSeptets(body, paddingBits, langIndex, langShiftIndex);
        break;
      case PDU_DCS_MSG_CODING_8BITS_ALPHABET:
        
        break;
      case PDU_DCS_MSG_CODING_16BITS_ALPHABET:
        this.writeUCS2String(body);
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
  RIL.handleDOMMessage(event.data);
};

onerror = function onerror(event) {
  debug("RIL Worker error" + event.message + "\n");
};
