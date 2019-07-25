





































"use strict";

importScripts("ril_consts.js", "systemlibs.js");


let DEBUG = DEBUG_WORKER;

const INT32_MAX   = 2147483647;
const UINT8_SIZE  = 1;
const UINT16_SIZE = 2;
const UINT32_SIZE = 4;
const PARCEL_SIZE_SIZE = UINT32_SIZE;

const PDU_HEX_OCTET_SIZE = 4;

const DEFAULT_EMERGENCY_NUMBERS = ["112", "911"];

let RILQUIRKS_CALLSTATE_EXTRA_UINT32 = libcutils.property_get("ro.moz.ril.callstate_extra_int");


let RILQUIRKS_V5_LEGACY = libcutils.property_get("ro.moz.ril.v5_legacy");
let RILQUIRKS_REQUEST_USE_DIAL_EMERGENCY_CALL = libcutils.property_get("ro.moz.ril.dial_emergency_call");
let RILQUIRKS_MODEM_DEFAULTS_TO_EMERGENCY_MODE = libcutils.property_get("ro.moz.ril.emergency_by_default");
let RILQUIRKS_SIM_APP_STATE_EXTRA_FIELDS = libcutils.property_get("ro.moz.ril.simstate_extra_field");


let PENDING_NETWORK_TYPE = {};













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
    
    
    
    this.readStringDelimiter(string_len);
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
  
  readStringDelimiter: function readStringDelimiter(length) {
    let delimiter = this.readUint16();
    if (!(length & 1)) {
      delimiter |= this.readUint16();
    }
    if (DEBUG) {
      if (delimiter != 0) {
        debug("Something's wrong, found string delimiter: " + delimiter);
      }
    }
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
    
    
    
    this.writeStringDelimiter(value.length);
  },

  writeStringList: function writeStringList(strings) {
    this.writeUint32(strings.length);
    for (let i = 0; i < strings.length; i++) {
      this.writeString(strings[i]);
    }
  },
  
  writeStringDelimiter: function writeStringDelimiter(length) {
    this.writeUint16(0);
    if (!(length & 1)) {
      this.writeUint16(0);
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
  _isInitialRadioState: true,

  



  iccStatus: null,

  


  cardState: null,

  


  IMEI: null,
  IMEISV: null,
  SMSC: null,

  


  iccInfo: {},

  


  aid: null,

  networkSelectionMode: null,

  voiceRegistrationState: {},
  dataRegistrationState: {},

  


  operator: null,

  


  basebandVersion: null,

  


  currentCalls: {},

  


  currentDataCalls: {},

  




  _receivedSmsSegmentsMap: {},

  


  _pendingSentSmsMap: {},

  



  _processingNetworkInfo: false,

  


  _pendingNetworkInfo: {rilMessageType: "networkinfochanged"},

  


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

  











  parseInt: function RIL_parseInt(string, defaultValue, radix) {
    let number = parseInt(string, radix || 10);
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

   


  iccUnlockCardLock: function iccUnlockCardLock(options) {
    switch (options.lockType) {
      case "pin":
        this.enterICCPIN(options);
        break;
      case "pin2":
        this.enterICCPIN2(options);
        break;
      case "puk":
        this.enterICCPUK(options);
        break;
      case "puk2":
        this.enterICCPUK2(options);
        break;
      default:
        options.errorMsg = "Unsupported Card Lock.";
        options.success = false;
        this.sendDOMMessage(options);
    }
  },

  







  enterICCPIN: function enterICCPIN(options) {
    Buf.newParcel(REQUEST_ENTER_SIM_PIN, options);
    Buf.writeUint32(RILQUIRKS_V5_LEGACY ? 1 : 2);
    Buf.writeString(options.pin);
    if (!RILQUIRKS_V5_LEGACY) {
      Buf.writeString(options.aid ? options.aid : this.aid);
    }
    Buf.sendParcel();
  },

  







  enterICCPIN2: function enterICCPIN2(options) {
    Buf.newParcel(REQUEST_ENTER_SIM_PIN2, options);
    Buf.writeUint32(RILQUIRKS_V5_LEGACY ? 1 : 2);
    Buf.writeString(options.pin);
    if (!RILQUIRKS_V5_LEGACY) {
      Buf.writeString(options.aid ? options.aid : this.aid);
    }
    Buf.sendParcel();
  },

   


  iccSetCardLock: function iccSetCardLock(options) {
    if (options.newPin !== undefined) {
      switch (options.lockType) {
        case "pin":
          this.changeICCPIN(options);
          break;
        case "pin2":
          this.changeICCPIN2(options);
          break;
        default:
          options.errorMsg = "Unsupported Card Lock.";
          options.success = false;
          this.sendDOMMessage(options);
      }
    } else { 
      if (options.lockType != "pin") {
        options.errorMsg = "Unsupported Card Lock.";
        options.success = false;
        this.sendDOMMessage(options);
        return;
      }
      this.setICCPinLock(options);
    }
  },

  









  changeICCPIN: function changeICCPIN(options) {
    Buf.newParcel(REQUEST_CHANGE_SIM_PIN, options);
    Buf.writeUint32(RILQUIRKS_V5_LEGACY ? 2 : 3);
    Buf.writeString(options.pin);
    Buf.writeString(options.newPin);
    if (!RILQUIRKS_V5_LEGACY) {
      Buf.writeString(options.aid ? options.aid : this.aid);
    }
    Buf.sendParcel();
  },

  









  changeICCPIN2: function changeICCPIN2(options) {
    Buf.newParcel(REQUEST_CHANGE_SIM_PIN2, options);
    Buf.writeUint32(RILQUIRKS_V5_LEGACY ? 2 : 3);
    Buf.writeString(options.pin);
    Buf.writeString(options.newPin);
    if (!RILQUIRKS_V5_LEGACY) {
      Buf.writeString(options.aid ? options.aid : this.aid);
    }
    Buf.sendParcel();
  },
  









   enterICCPUK: function enterICCPUK(options) {
     Buf.newParcel(REQUEST_ENTER_SIM_PUK, options);
     Buf.writeUint32(RILQUIRKS_V5_LEGACY ? 2 : 3);
     Buf.writeString(options.puk);
     Buf.writeString(options.newPin);
     if (!RILQUIRKS_V5_LEGACY) {
       Buf.writeString(options.aid ? options.aid : this.aid);
     }
     Buf.sendParcel();
   },

  









   enterICCPUK2: function enterICCPUK2(options) {
     Buf.newParcel(REQUEST_ENTER_SIM_PUK2, options);
     Buf.writeUint32(RILQUIRKS_V5_LEGACY ? 2 : 3);
     Buf.writeString(options.puk);
     Buf.writeString(options.newPin);
     if (!RILQUIRKS_V5_LEGACY) {
       Buf.writeString(options.aid ? options.aid : this.aid);
     }
     Buf.sendParcel();
   },

  


  iccGetCardLock: function iccGetCardLock(options) {
    switch (options.lockType) {
      case "pin":
        this.getICCPinLock(options);
        break;
      default:
        options.errorMsg = "Unsupported Card Lock.";
        options.success = false;
        this.sendDOMMessage(options);
    }
  },

  





  getICCPinLock: function getICCPinLock(options) {
    options.facility = ICC_CB_FACILITY_SIM;
    options.password = ""; 
    options.serviceClass = ICC_SERVICE_CLASS_VOICE |
                           ICC_SERVICE_CLASS_DATA  |
                           ICC_SERVICE_CLASS_FAX;
    this.queryICCFacilityLock(options);
  },

  











  queryICCFacilityLock: function queryICCFacilityLock(options) {
    Buf.newParcel(REQUEST_QUERY_FACILITY_LOCK, options);
    Buf.writeUint32(RILQUIRKS_V5_LEGACY ? 3 : 4);
    Buf.writeString(options.facility);
    Buf.writeString(options.password);
    Buf.writeString(options.serviceClass.toString());
    if (!RILQUIRKS_V5_LEGACY) {
      Buf.writeString(options.aid ? options.aid : this.aid);
    }
    Buf.sendParcel();
  },

  









  setICCPinLock: function setICCPinLock(options) {
    options.facility = ICC_CB_FACILITY_SIM;
    options.enabled = options.enabled;
    options.password = options.pin;
    options.serviceClass = ICC_SERVICE_CLASS_VOICE |
                           ICC_SERVICE_CLASS_DATA  |
                           ICC_SERVICE_CLASS_FAX;
    this.setICCFacilityLock(options);
  },

  













  setICCFacilityLock: function setICCFacilityLock(options) {
    Buf.newParcel(REQUEST_SET_FACILITY_LOCK, options);
    Buf.writeUint32(RILQUIRKS_V5_LEGACY ? 3 : 4);
    Buf.writeString(options.facility);
    Buf.writeString(options.enabled ? "1" : "0");
    Buf.writeString(options.password);
    Buf.writeString(options.serviceClass.toString());
    if (!RILQUIRKS_V5_LEGACY) {
      Buf.writeString(options.aid ? options.aid : this.aid);
    }
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
    Buf.writeString(options.pin2 ? options.pin2 : null);
    if (!RILQUIRKS_V5_LEGACY) {
      Buf.writeString(options.aid ? options.aid : this.aid);
    }
    Buf.sendParcel();
  },

  


  fetchICCRecords: function fetchICCRecords() {
    this.getIMSI();
    this.getMSISDN();
    this.getAD();
    this.getUST();
    this.getMBDN();
  },

  


  _handleICCInfoChange: function _handleICCInfoChange() {
    this.iccInfo.rilMessageType = "iccinfochange";
    this.sendDOMMessage(this.iccInfo);
  },

  





  getIMSI: function getIMSI(aid) {
    if (RILQUIRKS_V5_LEGACY) {
      Buf.simpleRequest(REQUEST_GET_IMSI);
      return;
    }
    let token = Buf.newParcel(REQUEST_GET_IMSI);
    Buf.writeUint32(1);
    Buf.writeString(aid ? aid : this.aid);
    Buf.sendParcel();
  },

  


  getMSISDN: function getMSISDN() {
    function callback(options) {
      let parseCallback = function parseCallback(msisdn) {
        if (this.iccInfo.msisdn === msisdn.number) {
          return;
        }
        this.iccInfo.msisdn = msisdn.number;
        if (DEBUG) debug("MSISDN: " + this.iccInfo.msisdn);
        this._handleICCInfoChange();
      }
      this.parseDiallingNumber(options, parseCallback);
    }

    this.iccIO({
      command:   ICC_COMMAND_GET_RESPONSE,
      fileId:    ICC_EF_MSISDN,
      pathId:    this._getPathIdForICCRecord(ICC_EF_MSISDN),
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
      this.iccInfo.ad = GsmPDUHelper.readHexOctetArray(len);
      Buf.readStringDelimiter(length);

      if (DEBUG) {
        let str = "";
        for (let i = 0; i < this.iccInfo.ad.length; i++) {
          str += this.iccInfo.ad[i] + ", ";
        }
        debug("AD: " + str);
      }

      if (this.iccInfo.imsi) {
        
        this.iccInfo.mcc = parseInt(this.iccInfo.imsi.substr(0,3));
        
        this.iccInfo.mnc = parseInt(this.iccInfo.imsi.substr(3, this.iccInfo.ad[3]));
        if (DEBUG) debug("MCC: " + this.iccInfo.mcc + " MNC: " + this.iccInfo.mnc);
        this._handleICCInfoChange();
      }
    }

    this.iccIO({
      command:   ICC_COMMAND_GET_RESPONSE,
      fileId:    ICC_EF_AD,
      pathId:    this._getPathIdForICCRecord(ICC_EF_AD),
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
    return this.iccInfo.ust &&
           (index < this.iccInfo.ust.length) &&
           (this.iccInfo.ust[index] & bitmask);
  },

  


  getUST: function getUST() {
    function callback() {
      let length = Buf.readUint32();
      
      let len = length / 2;
      this.iccInfo.ust = GsmPDUHelper.readHexOctetArray(len);
      Buf.readStringDelimiter(length);
      
      if (DEBUG) {
        let str = "";
        for (let i = 0; i < this.iccInfo.ust.length; i++) {
          str += this.iccInfo.ust[i] + ", ";
        }
        debug("UST: " + str);
      }
    }

    this.iccIO({
      command:   ICC_COMMAND_GET_RESPONSE,
      fileId:    ICC_EF_UST,
      pathId:    this._getPathIdForICCRecord(ICC_EF_UST),
      p1:        0, 
      p2:        0, 
      p3:        GET_RESPONSE_EF_SIZE_BYTES,
      data:      null,
      pin2:      null,
      type:      EF_TYPE_TRANSPARENT,
      callback:  callback,
    });
  },

  












  parseDiallingNumber: function parseDiallingNumber(options,
                                                    addCallback,
                                                    finishCallback) {
    let ffLen; 
    let length = Buf.readUint32();

    let alphaLen = options.recordSize - MSISDN_FOOTER_SIZE_BYTES;
    let alphaId = GsmPDUHelper.readAlphaIdentifier(alphaLen);

    let numLen = GsmPDUHelper.readHexOctet();
    if (numLen != 0xff) {
      if (numLen > MSISDN_MAX_NUMBER_SIZE_BYTES) {
        debug("invalid length of BCD number/SSC contents - " + numLen);
        return;
      }

      if (addCallback) {
        addCallback.call(this, {alphaId: alphaId,
                                number: GsmPDUHelper.readDiallingNumber(numLen)});
      }

      ffLen = length / 2 - alphaLen - numLen - 1; 
    } else {
      ffLen = MSISDN_FOOTER_SIZE_BYTES - 1; 
    }

    
    for (let i = 0; i < ffLen; i++) {
      GsmPDUHelper.readHexOctet();
    }
    
    Buf.readStringDelimiter(length);
    
    if (options.loadAll &&
        options.p1 < options.totalRecords) {
      options.p1++;
      this.iccIO(options);
    } else {
      if (finishCallback) {
        finishCallback.call(this);
      }
    }
  },
  
  





  getFDN: function getFDN(options) {
    function callback(options) {
      function add(contact) {
        this.iccInfo.fdn.push(contact);
      };
      function finish() {
        if (DEBUG) {
          for (let i = 0; i < this.iccInfo.fdn.length; i++) {
            debug("FDN[" + i + "] alphaId = " + this.iccInfo.fdn[i].alphaId +
                                " number = " + this.iccInfo.fdn[i].number);
          }
        }
        this.sendDOMMessage({rilMessageType: "icccontacts",
                             contactType: "FDN",
                             contacts: this.iccInfo.fdn,
                             requestId: options.requestId});
      };
      this.parseDiallingNumber(options, add, finish);
    }
    
    this.iccInfo.fdn = [];
    this.iccIO({
      command:   ICC_COMMAND_GET_RESPONSE,
      fileId:    ICC_EF_FDN,
      pathId:    this._getPathIdForICCRecord(ICC_EF_FDN),
      p1:        0, 
      p2:        0, 
      p3:        GET_RESPONSE_EF_SIZE_BYTES,
      data:      null,
      pin2:      null,
      type:      EF_TYPE_LINEAR_FIXED,
      callback:  callback,
      loadAll:   true,
      requestId: options.requestId
    });
  },

  







  getADN: function getADN(options) {
    function callback(options) {
      function add(contact) {
        this.iccInfo.adn.push(contact);
      };
      function finish() {
        if (DEBUG) {
          for (let i = 0; i < this.iccInfo.adn.length; i++) {
            debug("ADN[" + i + "] alphaId = " + this.iccInfo.adn[i].alphaId +
                                " number = " + this.iccInfo.adn[i].number);
          }
        }
        this.sendDOMMessage({rilMessageType: "icccontacts",
                             contactType: "ADN",
                             contacts: this.iccInfo.adn,
                             requestId: options.requestId});
      };
      this.parseDiallingNumber(options, add, finish);
    }

    this.iccInfo.adn = [];
    this.iccIO({
      command:   ICC_COMMAND_GET_RESPONSE,
      fileId:    options.fileId,
      pathId:    this._getPathIdForICCRecord(options.fileId),
      p1:        0, 
      p2:        0, 
      p3:        GET_RESPONSE_EF_SIZE_BYTES,
      data:      null,
      pin2:      null,
      type:      EF_TYPE_LINEAR_FIXED,
      callback:  callback,
      loadAll:   true,
      requestId: options.requestId
    });
  },

   




  getMBDN: function getMBDN() {
    function callback(options) {
      let parseCallback = function parseCallback(contact) {
        if (DEBUG) {
          debug("MBDN, alphaId="+contact.alphaId+" number="+contact.number);
        }
        if (this.iccInfo.mbdn != contact.number) {
          this.iccInfo.mbdn = contact.number;
          contact.rilMessageType = "iccmbdn";
          this.sendDOMMessage(contact);
        }
      };

      this.parseDiallingNumber(options, parseCallback);
    }

    this.iccIO({
      command:   ICC_COMMAND_GET_RESPONSE,
      fileId:    ICC_EF_MBDN,
      pathId:    this._getPathIdForICCRecord(ICC_EF_MBDN),
      p1:        0, 
      p2:        0, 
      p3:        GET_RESPONSE_EF_SIZE_BYTES,
      data:      null,
      pin2:      null,
      type:      EF_TYPE_LINEAR_FIXED,
      callback:  callback,
    });
  },

  decodeSimTlvs: function decodeSimTlvs(tlvsLen) {
    let index = 0;
    let tlvs = [];
    while (index < tlvsLen) {
      let simTlv = {
        tag : GsmPDUHelper.readHexOctet(),
        length : GsmPDUHelper.readHexOctet(),
      };
      simTlv.value = GsmPDUHelper.readHexOctetArray(simTlv.length)
      tlvs.push(simTlv);
      index += simTlv.length + 2 ;
    }
    return tlvs;
  },

  _searchForIccUsimTag: function _searchForIccUsimTag(tlvs, tag) {
    for (let i = 0; i < tlvs.length; i++) {
      if (tlvs[i].tag == tag) {
        return tlvs[i];
      }
    }
    return null;
  },

  





  getPBR: function getPBR(options) {
    function callback(options) {
      let bufLen = Buf.readUint32();

      let tag = GsmPDUHelper.readHexOctet();
      let length = GsmPDUHelper.readHexOctet();
      let value = this.decodeSimTlvs(length);

      let adn = this._searchForIccUsimTag(value, ICC_USIM_EFADN_TAG);
      let adnEfid = (adn.value[0] << 8) | adn.value[1];
      this.getADN({fileId: adnEfid,
                   requestId: options.requestId});

      Buf.readStringDelimiter(bufLen);
    }

    this.iccIO({
      command:   ICC_COMMAND_GET_RESPONSE,
      fileId:    ICC_EF_PBR,
      pathId:    this._getPathIdForICCRecord(ICC_EF_PBR),
      p1:        0, 
      p2:        0, 
      p3:        GET_RESPONSE_EF_SIZE_BYTES,
      data:      null,
      pin2:      null,
      type:      EF_TYPE_LINEAR_FIXED,
      callback:  callback,
      requestId: options.requestId,
    });
  },

  





  setRadioPower: function setRadioPower(options) {
    Buf.newParcel(REQUEST_RADIO_POWER);
    Buf.writeUint32(1);
    Buf.writeUint32(options.on ? 1 : 0);
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

  





  setPreferredNetworkType: function setPreferredNetworkType(network_type) {
    Buf.newParcel(REQUEST_SET_PREFERRED_NETWORK_TYPE);
    Buf.writeUint32(network_type);
    Buf.sendParcel();
  },

  


  requestNetworkInfo: function requestNetworkInfo() {
    if (this._processingNetworkInfo) {
      if (DEBUG) debug("Network info requested, but we're already requesting network info.");
      return;
    }

    if (DEBUG) debug("Requesting network info");

    this._processingNetworkInfo = true;
    this.getVoiceRegistrationState();
    this.getDataRegistrationState(); 
    this.getOperator();
    this.getNetworkSelectionMode();
  },

  


  getAvailableNetworks: function getAvailableNetworks(options) {
    if (DEBUG) debug("Getting available networks");
    Buf.newParcel(REQUEST_QUERY_AVAILABLE_NETWORKS, options);
    Buf.sendParcel();
  },

  


  getNetworkSelectionMode: function getNetworkSelectionMode(options) {
    if (DEBUG) debug("Getting network selection mode");
    Buf.simpleRequest(REQUEST_QUERY_NETWORK_SELECTION_MODE, options);
  },

  


  selectNetworkAuto: function selectNetworkAuto(options) {
    if (DEBUG) debug("Setting automatic network selection");
    Buf.simpleRequest(REQUEST_SET_NETWORK_SELECTION_AUTOMATIC, options);
  },

  


  selectNetwork: function selectNetwork(options) {
    if (DEBUG) {
      debug("Setting manual network selection: " + options.mcc + options.mnc);
    }

    let numeric = String(options.mcc) + options.mnc;
    Buf.newParcel(REQUEST_SET_NETWORK_SELECTION_MANUAL, options);
    Buf.writeString(numeric);
    Buf.sendParcel();
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
    if (this.voiceRegistrationState.emergencyCallsOnly ||
        options.isDialEmergency) {
      if (!this._isEmergencyNumber(options.number)) {
        
        options.callIndex = -1;
        options.rilMessageType = "callError";
        options.error =
          RIL_CALL_FAILCAUSE_TO_GECKO_CALL_ERROR[CALL_FAIL_UNOBTAINABLE_NUMBER];
        this.sendDOMMessage(options);
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
    if (!call) {
      return;
    }

    switch (call.state) {
      case CALL_STATE_ACTIVE:
      case CALL_STATE_DIALING:
      case CALL_STATE_ALERTING:
        Buf.newParcel(REQUEST_HANGUP);
        Buf.writeUint32(1);
        Buf.writeUint32(options.callIndex);
        Buf.sendParcel();
        break;
      case CALL_STATE_HOLDING:
        Buf.simpleRequest(REQUEST_HANGUP_WAITING_OR_BACKGROUND);
        break;
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

  


  getSMSCAddress: function getSMSCAddress() {
    Buf.simpleRequest(REQUEST_GET_SMSC_ADDRESS);
  },

  





  setSMSCAddress: function setSMSCAddress(options) {
    Buf.newParcel(REQUEST_SET_SMSC_ADDRESS);
    Buf.writeString(options.SMSC);
    Buf.sendParcel();
  },

  





















  setupDataCall: function setupDataCall(options) {
    let token = Buf.newParcel(REQUEST_SETUP_DATA_CALL, options);
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

    let token = Buf.newParcel(REQUEST_DEACTIVATE_DATA_CALL, options);
    Buf.writeUint32(2);
    Buf.writeString(options.cid);
    Buf.writeString(options.reason || DATACALL_DEACTIVATE_NO_REASON);
    Buf.sendParcel();

    datacall.state = GECKO_NETWORK_STATE_DISCONNECTING;
    this.sendDOMMessage(datacall);
  },

  


  getDataCallList: function getDataCallList() {
    Buf.simpleRequest(REQUEST_DATA_CALL_LIST);
  },

  


  getFailCauseCode: function getFailCauseCode(options) {
    Buf.simpleRequest(REQUEST_LAST_CALL_FAIL_CAUSE, options);
  },

  






   sendUSSD: function sendUSSD(options) {
     Buf.newParcel(REQUEST_SEND_USSD, options);
     Buf.writeString(options.ussd);
     Buf.sendParcel();
   },

  


   cancelUSSD: function cancelUSSD(options) {
     Buf.simpleRequest(REQUEST_CANCEL_USSD, options);
   },

  





  stkHandleCallSetup: function stkHandleCallSetup(options) {
     Buf.newParcel(REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM, options);
     Buf.writeUint32(1);
     Buf.writeUint32(options.hasConfirmed ? 1 : 0);
     Buf.sendParcel();
  },

  











  sendStkTerminalResponse: function sendStkTerminalResponse(response) {
    if (response.hasConfirmed !== undefined) {
      this.stkHandleCallSetup(response);
      return;
    }

    let token = Buf.newParcel(REQUEST_STK_SEND_TERMINAL_RESPONSE);
    let textLen = 0;
    if (response.resultCode != STK_RESULT_HELP_INFO_REQUIRED) {
      if (response.isYesNo) {
        textLen = 1;
      } else if (response.input) {
        if (response.isUCS2) {
          textLen = response.input.length * 2;
        } else if (response.isPacked) {
          let bits = response.input.length * 7;
          textLen = bits * 7 / 8 + (bits % 8 ? 1 : 0);
        } else {
          textLen = response.input.length;
        }
      }
    }

    
    let size = (5 + 
                4 + 
                3 + 
                (response.itemIdentifier ? 3 : 0) +
                (textLen ? textLen + 3 : 0)) * 2;
    Buf.writeUint32(size);

    
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_COMMAND_DETAILS |
                               COMPREHENSIONTLV_FLAG_CR);
    GsmPDUHelper.writeHexOctet(3);
    if (response.command) {
      GsmPDUHelper.writeHexOctet(response.command.commandNumber);
      GsmPDUHelper.writeHexOctet(response.command.typeOfCommand);
      GsmPDUHelper.writeHexOctet(response.command.commandQualifier);
    } else {
      GsmPDUHelper.writeHexOctet(0x00);
      GsmPDUHelper.writeHexOctet(0x00);
      GsmPDUHelper.writeHexOctet(0x00);
    }

    
    
    
    
    
    
    
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_DEVICE_ID);
    GsmPDUHelper.writeHexOctet(2);
    GsmPDUHelper.writeHexOctet(STK_DEVICE_ID_ME);
    GsmPDUHelper.writeHexOctet(STK_DEVICE_ID_SIM);

    
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_RESULT |
                               COMPREHENSIONTLV_FLAG_CR);
    GsmPDUHelper.writeHexOctet(1);
    GsmPDUHelper.writeHexOctet(response.resultCode);

    
    if (response.itemIdentifier) {
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_ITEM_ID |
                                 COMPREHENSIONTLV_FLAG_CR);
      GsmPDUHelper.writeHexOctet(1);
      GsmPDUHelper.writeHexOctet(response.itemIdentifier);
    }

    
    if (response.resultCode != STK_RESULT_HELP_INFO_REQUIRED) {
      let text;
      if (response.isYesNo !== undefined) {
        
        
        
        
        
        text = response.isYesNo ? 0x01 : 0x00;
      } else {
        text = response.input;
      }

      if (text) {
        GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_TEXT_STRING |
                                   COMPREHENSIONTLV_FLAG_CR);
        GsmPDUHelper.writeHexOctet(textLen + 1); 
        let coding = response.isUCS2 ? STK_TEXT_CODING_UCS2 :
                       (response.isPacked ? STK_TEXT_CODING_GSM_7BIT_PACKED :
                          STK_TEXT_CODING_GSM_8BIT);
        GsmPDUHelper.writeHexOctet(coding);

        
        switch (coding) {
          case STK_TEXT_CODING_UCS2:
            GsmPDUHelper.writeUCS2String(text);
            break;
          case STK_TEXT_CODING_GSM_7BIT_PACKED:
            GsmPDUHelper.writeStringAsSeptets(text, 0, 0, 0);
            break;
          case STK_TEXT_CODING_GSM_8BIT:
            for (let i = 0; i < textLen; i++) {
              GsmPDUHelper.writeHexOctet(text.charCodeAt(i));
            }
            break;
        }
      }
    }

    Buf.writeUint32(0);
    Buf.sendParcel();
  },

  





  sendStkMenuSelection: function sendStkMenuSelection(command) {
    this.sendICCEnvelopeCommand(command);
  },

  





  sendICCEnvelopeCommand: function sendICCEnvelopeCommand(options) {
    let token = Buf.newParcel(REQUEST_STK_SEND_ENVELOPE_COMMAND);
    let berLen = 4 + 
                 (options.itemIdentifier ? 3 : 0) +
                 (options.helpRequested ? 2 : 0);
    let size = (2 + berLen) * 2;

    Buf.writeUint32(size);

    
    GsmPDUHelper.writeHexOctet(BER_MENU_SELECTION_TAG);
    GsmPDUHelper.writeHexOctet(berLen);

    
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_DEVICE_ID |
                               COMPREHENSIONTLV_FLAG_CR);
    GsmPDUHelper.writeHexOctet(2);
    GsmPDUHelper.writeHexOctet(STK_DEVICE_ID_KEYPAD);
    GsmPDUHelper.writeHexOctet(STK_DEVICE_ID_SIM);

    
    if (options.itemIdentifier) {
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_ITEM_ID |
                                 COMPREHENSIONTLV_FLAG_CR);
      GsmPDUHelper.writeHexOctet(1);
      GsmPDUHelper.writeHexOctet(options.itemIdentifier);
    }

    
    if (options.helpRequested) {
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_HELP_REQUEST |
                                 COMPREHENSIONTLV_FLAG_CR);
      GsmPDUHelper.writeHexOctet(0);
      
    }

    Buf.writeUint32(0);
    Buf.sendParcel();
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
      this.sendDOMMessage({rilMessageType: "cardstatechange",
                           cardState: this.cardState});
      return;
    }

    
    let index = iccStatus.gsmUmtsSubscriptionAppIndex;
    let app = iccStatus.apps[index];
    if (!app) {
      if (DEBUG) {
        debug("Subscription application is not present in iccStatus.");
      }
      if (this.cardState == GECKO_CARDSTATE_ABSENT) {
        return;
      }
      this.cardState = GECKO_CARDSTATE_ABSENT;
      this.operator = null;
      this.sendDOMMessage({rilMessageType: "cardstatechange",
                           cardState: this.cardState});
      return;
    }
    
    this.aid = app.aid;

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

    
    this.requestNetworkInfo();
    this.getSignalStrength();
    if (newCardState == GECKO_CARDSTATE_READY) {
      this.fetchICCRecords();
    }

    this.cardState = newCardState;
    this.sendDOMMessage({rilMessageType: "cardstatechange",
                         cardState: this.cardState});
  },

  







  _getPathIdForICCRecord: function _getPathIdForICCRecord(fileId) {
    let index = this.iccStatus.gsmUmtsSubscriptionAppIndex;
    if (index == -1) {
      return null;
    }
    let app = this.iccStatus.apps[index];
    if (!app) {
      return null;
    }

    switch (app.app_type) {
      case CARD_APPTYPE_SIM:
        switch (fileId) {
          case ICC_EF_ADN:
          case ICC_EF_FDN:
          case ICC_EF_MSISDN:
            return EF_PATH_MF_SIM + EF_PATH_DF_TELECOM;

          case ICC_EF_AD:
          case ICC_EF_MBDN:
          case ICC_EF_UST:
            return EF_PATH_MF_SIM + EF_PATH_DF_GSM;
          case ICC_EF_PBR:
            return EF_PATH_MF_SIM + EF_PATH_DF_TELECOM + EF_PATH_DF_PHONEBOOK;
        }
      case CARD_APPTYPE_USIM:
        switch (fileId) {
          case ICC_EF_AD:
          case ICC_EF_MBDN:
          case ICC_EF_UST:
          case ICC_EF_MSISDN:
            return EF_PATH_MF_SIM + EF_PATH_ADF_USIM;
          case ICC_EF_ADN:
          case ICC_EF_FDN:
            return EF_PATH_MF_SIM + EF_PATH_DF_TELECOM;
          case ICC_EF_PBR:
            return EF_PATH_MF_SIM + EF_PATH_DF_TELECOM + EF_PATH_DF_PHONEBOOK;
          default:
            
	    
	    
            return EF_PATH_MF_SIM + EF_PATH_DF_TELECOM + EF_PATH_DF_PHONEBOOK;
        }
    }
    return null;
  },

   


  _processEnterAndChangeICCResponses: function _processEnterAndChangeICCResponses(length, options) {
    options.success = options.rilRequestError == 0;
    if (!options.success) {
      options.errorMsg = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
    }
    options.retryCount = length ? Buf.readUint32List()[0] : -1;
    this.sendDOMMessage(options);
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

    
    options.recordSize = GsmPDUHelper.readHexOctet();
    options.totalRecords = fileSize / options.recordSize;

    Buf.readStringDelimiter(length);

    switch (options.type) {
      case EF_TYPE_LINEAR_FIXED:
        
        options.command = ICC_COMMAND_READ_RECORD;
        options.p1 = 1; 
        options.p2 = READ_RECORD_ABSOLUTE_MODE;
        options.p3 = options.recordSize;
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
      options.callback.call(this, options);
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

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  _sendNetworkInfoMessage: function _sendNetworkInfoMessage(type, message) {
    if (!this._processingNetworkInfo) {
      
      
      this.sendDOMMessage(message);
      return;
    }

    if (DEBUG) debug("Queuing " + type + " network info message: " + JSON.stringify(message));
    this._pendingNetworkInfo[type] = message;
  },

  _receivedNetworkInfo: function _receivedNetworkInfo(type) {
    if (DEBUG) debug("Received " + type + " network info.");
    if (!this._processingNetworkInfo) {
      return;
    }

    let pending = this._pendingNetworkInfo;

    
    if (!(type in pending)) {
      pending[type] = PENDING_NETWORK_TYPE;
    }

    
    
    for (let i = 0; i < NETWORK_INFO_MESSAGE_TYPES.length; i++) {
      let type = NETWORK_INFO_MESSAGE_TYPES[i];
      if (!(type in pending)) {
        if (DEBUG) debug("Still missing some more network info, not notifying main thread.");
        return;
      }
    }

    
    
    
    for (let key in pending) {
      if (pending[key] == PENDING_NETWORK_TYPE) {
        delete pending[key];
      }
    }

    if (DEBUG) debug("All pending network info has been received: " + JSON.stringify(pending));

    
    
    setTimeout(this._sendPendingNetworkInfo, 0);
  },

  _sendPendingNetworkInfo: function _sendPendingNetworkInfo() {
    RIL.sendDOMMessage(RIL._pendingNetworkInfo);

    RIL._processingNetworkInfo = false;
    for (let i = 0; i < NETWORK_INFO_MESSAGE_TYPES.length; i++) {
      delete RIL._pendingNetworkInfo[NETWORK_INFO_MESSAGE_TYPES[i]];
    }
  },

  




  _processCREG: function _processCREG(curState, newState) {
    let changed = false;

    let regState = RIL.parseInt(newState[0], NETWORK_CREG_STATE_UNKNOWN);
    if (curState.regState != regState) {
      changed = true;
      curState.regState = regState;

      curState.state = NETWORK_CREG_TO_GECKO_MOBILE_CONNECTION_STATE[regState];
      curState.connected = regState == NETWORK_CREG_STATE_REGISTERED_HOME ||
                           regState == NETWORK_CREG_STATE_REGISTERED_ROAMING;
      curState.roaming = regState == NETWORK_CREG_STATE_REGISTERED_ROAMING;
      curState.emergencyCallsOnly =
        (regState >= NETWORK_CREG_STATE_NOT_SEARCHING_EMERGENCY_CALLS) &&
        (regState <= NETWORK_CREG_STATE_UNKNOWN_EMERGENCY_CALLS);
      if (RILQUIRKS_MODEM_DEFAULTS_TO_EMERGENCY_MODE) {
        curState.emergencyCallsOnly = !curState.connected;
      }
    }

    if (!curState.cell) {
      curState.cell = {};
    }

    
    
    let lac = RIL.parseInt(newState[1], -1, 16);
    if (curState.cell.gsmLocationAreaCode !== lac) {
      curState.cell.gsmLocationAreaCode = lac;
      changed = true;
    }

    let cid = RIL.parseInt(newState[2], -1, 16);
    if (curState.cell.gsmCellId !== cid) {
      curState.cell.gsmCellId = cid;
      changed = true;
    }

    let radioTech = RIL.parseInt(newState[3], NETWORK_CREG_TECH_UNKNOWN);
    if (curState.radioTech != radioTech) {
      changed = true;
      curState.radioTech = radioTech;
      curState.type = GECKO_RADIO_TECH[radioTech] || null;
    }
    return changed;
  },

  _processVoiceRegistrationState: function _processVoiceRegistrationState(state) {
    let rs = this.voiceRegistrationState;
    let stateChanged = this._processCREG(rs, state);
    if (stateChanged && rs.connected) {
      RIL.getSMSCAddress();
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
      rs.rilMessageType = "voiceregistrationstatechange";
      this._sendNetworkInfoMessage(NETWORK_INFO_VOICE_REGISTRATION_STATE, rs);
    }
  },

  _processDataRegistrationState: function _processDataRegistrationState(state) {
    let rs = this.dataRegistrationState;
    let stateChanged = this._processCREG(rs, state);
    if (stateChanged) {
      rs.rilMessageType = "dataregistrationstatechange";
      this._sendNetworkInfoMessage(NETWORK_INFO_DATA_REGISTRATION_STATE, rs);
    }
  },

  _processOperator: function _processOperator(operatorData) {
    if (operatorData.length < 3) {
      if (DEBUG) {
        debug("Expected at least 3 strings for operator.");
      }
    }

    if (!this.operator) {
      this.operator = {rilMessageType: "operatorchange"};
    }

    let [longName, shortName, networkTuple] = operatorData;
    let thisTuple = "" + this.operator.mcc + this.operator.mnc;

    if (this.operator.longName !== longName ||
        this.operator.shortName !== shortName ||
        thisTuple !== networkTuple) {

      this.operator.longName = longName;
      this.operator.shortName = shortName;
      this.operator.mcc = 0;
      this.operator.mnc = 0;

      
      
      
      if (DEBUG && !longName) {
        debug("Operator is currently unregistered");
      }

      if (longName && shortName && networkTuple) {
        try {
          this._processNetworkTuple(networkTuple, this.operator);
        } catch (e) {
          debug("Error processing operator tuple: " + e);
        }
      }

      this._sendNetworkInfoMessage(NETWORK_INFO_OPERATOR, this.operator);
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
          currentCall.isActive = this._isActiveCall(currentCall.state);
          this._handleChangedCallState(currentCall);
        }
      } else {
        
        
        delete this.currentCalls[currentCall.callIndex];
        this.getFailCauseCode(currentCall);
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
        newCall.isActive = this._isActiveCall(newCall.state);
        this._handleChangedCallState(newCall);
      }
    }

    
    
    this.muted = Object.getOwnPropertyNames(this.currentCalls).length == 0;
  },

  _handleChangedCallState: function _handleChangedCallState(changedCall) {
    let message = {rilMessageType: "callStateChange",
                   call: changedCall};
    this.sendDOMMessage(message);
  },

  _handleDisconnectedCall: function _handleDisconnectedCall(disconnectedCall) {
    let message = {rilMessageType: "callDisconnected",
                   call: disconnectedCall};
    this.sendDOMMessage(message);
  },

  _isActiveCall: function _isActiveCall(callState) {
    switch (callState) {
      case CALL_STATE_ACTIVE:
      case CALL_STATE_DIALING:
      case CALL_STATE_ALERTING:
        return true;
      case CALL_STATE_HOLDING:
      case CALL_STATE_INCOMING:
      case CALL_STATE_WAITING:
        return false;
    }
  },

  _processDataCallList: function _processDataCallList(datacalls, newDataCallOptions) {
    for each (let currentDataCall in this.currentDataCalls) {
      let updatedDataCall;
      if (datacalls) {
        updatedDataCall = datacalls[currentDataCall.cid];
        delete datacalls[currentDataCall.cid];
      }

      if (!updatedDataCall) {
        delete this.currentDataCalls[currentDataCall.callIndex];
        currentDataCall.state = GECKO_NETWORK_STATE_DISCONNECTED;
        currentDataCall.rilMessageType = "datacallstatechange";
        this.sendDOMMessage(currentDataCall);
        continue;
      }

      this._setDataCallGeckoState(updatedDataCall);
      if (updatedDataCall.state != currentDataCall.state) {
        currentDataCall.status = updatedDataCall.status;
        currentDataCall.active = updatedDataCall.active;
        currentDataCall.state = updatedDataCall.state;
        currentDataCall.rilMessageType = "datacallstatechange";
        this.sendDOMMessage(currentDataCall);
      }
    }

    for each (let newDataCall in datacalls) {
      this.currentDataCalls[newDataCall.cid] = newDataCall;
      this._setDataCallGeckoState(newDataCall);
      if (newDataCallOptions) {
        newDataCall.radioTech = newDataCallOptions.radioTech;
        newDataCall.apn = newDataCallOptions.apn;
        newDataCall.user = newDataCallOptions.user;
        newDataCall.passwd = newDataCallOptions.passwd;
        newDataCall.chappap = newDataCallOptions.chappap;
        newDataCall.pdptype = newDataCallOptions.pdptype;
        newDataCallOptions = null;
      } else if (DEBUG) {
        debug("Unexpected new data call: " + JSON.stringify(newDataCall));
      }
      newDataCall.rilMessageType = "datacallstatechange";
      this.sendDOMMessage(newDataCall);
    }
  },

  _setDataCallGeckoState: function _setDataCallGeckoState(datacall) {
    switch (datacall.active) {
      case DATACALL_INACTIVE:
        datacall.state = GECKO_NETWORK_STATE_DISCONNECTED;
        break;
      case DATACALL_ACTIVE_DOWN:
      case DATACALL_ACTIVE_UP:
        datacall.state = GECKO_NETWORK_STATE_CONNECTED;
        break;
    }
  },

  _processNetworks: function _processNetworks() {
    let strings = Buf.readStringList();
    let networks = [];

    for (let i = 0; i < strings.length; i += 4) {
      let network = {
        longName: strings[i],
        shortName: strings[i + 1],
        mcc: 0, mnc: 0,
        state: null
      };

      let networkTuple = strings[i + 2];
      try {
        this._processNetworkTuple(networkTuple, network);
      } catch (e) {
        debug("Error processing operator tuple: " + e);
      }

      let state = strings[i + 3];
      if (state === NETWORK_STATE_UNKNOWN) {
        
        
        state = GECKO_QAN_STATE_UNKNOWN;
      }

      network.state = state;
      networks.push(network);
    }
    return networks;
  },

  





  _processNetworkTuple: function _processNetworkTuple(networkTuple, network) {
    let tupleLen = networkTuple.length;
    let mcc = 0, mnc = 0;

    if (tupleLen == 5 || tupleLen == 6) {
      mcc = parseInt(networkTuple.substr(0, 3), 10);
      if (isNaN(mcc)) {
        throw new Error("MCC could not be parsed from network tuple: " + networkTuple );
      }

      mnc = parseInt(networkTuple.substr(3), 10);
      if (isNaN(mnc)) {
        throw new Error("MNC could not be parsed from network tuple: " + networkTuple);
      }
    } else {
      throw new Error("Invalid network tuple (should be 5 or 6 digits): " + networkTuple);
    }

    network.mcc = mcc;
    network.mnc = mnc;
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

    
    Buf.readStringDelimiter(length);

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
      message.rilMessageType = "sms-received";
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
        rilMessageType: "sms-delivered",
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
    let method = this[message.rilMessageType];
    if (typeof method != "function") {
      if (DEBUG) {
        debug("Don't know what to do with message " + JSON.stringify(message));
      }
      return;
    }
    method.call(this, message);
  },

  


  enumerateCalls: function enumerateCalls(options) {
    if (DEBUG) debug("Sending all current calls");
    let calls = [];
    for each (let call in this.currentCalls) {
      calls.push(call);
    }
    options.calls = calls;
    this.sendDOMMessage(options);
  },

  


  enumerateDataCalls: function enumerateDataCalls() {
    let datacall_list = [];
    for each (let datacall in this.currentDataCalls) {
      datacall_list.push(datacall);
    }
    this.sendDOMMessage({rilMessageType: "datacalllist",
                         datacalls: datacall_list});
  },

  


  processStkProactiveCommand: function processStkProactiveCommand() {
    let length = Buf.readUint32();
    let berTlv = BerTlvHelper.decode(length / 2);
    Buf.readStringDelimiter(length);

    let ctlvs = berTlv.value;
    let ctlv = StkProactiveCmdHelper.searchForTag(
        COMPREHENSIONTLV_TAG_COMMAND_DETAILS, ctlvs);
    if (!ctlv) {
      RIL.sendStkTerminalResponse({
        resultCode: STK_RESULT_CMD_DATA_NOT_UNDERSTOOD});
      throw new Error("Can't find COMMAND_DETAILS ComprehensionTlv");
    }

    let cmdDetails = ctlv.value;
    if (DEBUG) {
      debug("commandNumber = " + cmdDetails.commandNumber +
           " typeOfCommand = " + cmdDetails.typeOfCommand.toString(16) +
           " commandQualifier = " + cmdDetails.commandQualifier);
    }

    let param = StkCommandParamsFactory.createParam(cmdDetails, ctlvs);
    if (param) {
      cmdDetails.rilMessageType = "stkcommand";
      cmdDetails.options = param;
      RIL.sendDOMMessage(cmdDetails);
    }
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
    if (RILQUIRKS_SIM_APP_STATE_EXTRA_FIELDS) {
      Buf.readUint32();
      Buf.readUint32();
      Buf.readUint32();
      Buf.readUint32();
    }
  }

  if (DEBUG) debug("iccStatus: " + JSON.stringify(iccStatus));
  this._processICCStatus(iccStatus);
};
RIL[REQUEST_ENTER_SIM_PIN] = function REQUEST_ENTER_SIM_PIN(length, options) {
  this._processEnterAndChangeICCResponses(length, options);
};
RIL[REQUEST_ENTER_SIM_PUK] = function REQUEST_ENTER_SIM_PUK(length, options) {
  this._processEnterAndChangeICCResponses(length, options);
};
RIL[REQUEST_ENTER_SIM_PIN2] = function REQUEST_ENTER_SIM_PIN2(length, options) {
  this._processEnterAndChangeICCResponses(length, options);
};
RIL[REQUEST_ENTER_SIM_PUK2] = function REQUEST_ENTER_SIM_PUK(length, options) {
  this._processEnterAndChangeICCResponses(length, options);
};
RIL[REQUEST_CHANGE_SIM_PIN] = function REQUEST_CHANGE_SIM_PIN(length, options) {
  this._processEnterAndChangeICCResponses(length, options);
};
RIL[REQUEST_CHANGE_SIM_PIN2] = function REQUEST_CHANGE_SIM_PIN2(length, options) {
  this._processEnterAndChangeICCResponses(length, options);
};
RIL[REQUEST_ENTER_NETWORK_DEPERSONALIZATION] = null;
RIL[REQUEST_GET_CURRENT_CALLS] = function REQUEST_GET_CURRENT_CALLS(length, options) {
  if (options.rilRequestError) {
    return;
  }

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

    call.isActive = false;

    calls[call.callIndex] = call;
  }
  this._processCalls(calls);
};
RIL[REQUEST_DIAL] = function REQUEST_DIAL(length, options) {
  if (options.rilRequestError) {
    
    options.callIndex = -1;
    this.getFailCauseCode(options);
    return;
  }
};
RIL[REQUEST_GET_IMSI] = function REQUEST_GET_IMSI(length, options) {
  if (options.rilRequestError) {
    return;
  }

  this.iccInfo.imsi = Buf.readString();
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
RIL[REQUEST_LAST_CALL_FAIL_CAUSE] = function REQUEST_LAST_CALL_FAIL_CAUSE(length, options) {
  let num = 0;
  if (length) {
    num = Buf.readUint32();
  }
  if (!num) {
    
    
    this._handleDisconnectedCall(options);
    return;
  }

  let failCause = Buf.readUint32();
  switch (failCause) {
    case CALL_FAIL_NORMAL:
      this._handleDisconnectedCall(options);
      break;
    case CALL_FAIL_BUSY:
      options.state = CALL_STATE_BUSY;
      this._handleChangedCallState(options);
      this._handleDisconnectedCall(options);
      break;
    case CALL_FAIL_UNOBTAINABLE_NUMBER:
    case CALL_FAIL_CONGESTION:
    case CALL_FAIL_ACM_LIMIT_EXCEEDED:
    case CALL_FAIL_CALL_BARRED:
    case CALL_FAIL_FDN_BLOCKED:
    case CALL_FAIL_IMSI_UNKNOWN_IN_VLR:
    case CALL_FAIL_IMEI_NOT_ACCEPTED:
    case CALL_FAIL_ERROR_UNSPECIFIED:
      options.rilMessageType = "callError";
      options.error = RIL_CALL_FAILCAUSE_TO_GECKO_CALL_ERROR[failCause];
      this.sendDOMMessage(options);
      break;
  }
};
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
  obj.rilMessageType = "signalstrengthchange";
  this.sendDOMMessage(obj);
};
RIL[REQUEST_VOICE_REGISTRATION_STATE] = function REQUEST_VOICE_REGISTRATION_STATE(length, options) {
  this._receivedNetworkInfo(NETWORK_INFO_VOICE_REGISTRATION_STATE);

  if (options.rilRequestError) {
    return;
  }

  let state = Buf.readStringList();
  if (DEBUG) debug("voice registration state: " + state);

  this._processVoiceRegistrationState(state);
};
RIL[REQUEST_DATA_REGISTRATION_STATE] = function REQUEST_DATA_REGISTRATION_STATE(length, options) {
  this._receivedNetworkInfo(NETWORK_INFO_DATA_REGISTRATION_STATE);

  if (options.rilRequestError) {
    return;
  }

  let state = Buf.readStringList();
  this._processDataRegistrationState(state);
};
RIL[REQUEST_OPERATOR] = function REQUEST_OPERATOR(length, options) {
  this._receivedNetworkInfo(NETWORK_INFO_OPERATOR);

  if (options.rilRequestError) {
    return;
  }

  let operatorData = Buf.readStringList();
  if (DEBUG) debug("Operator: " + operatorData);
  this._processOperator(operatorData);
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
          rilMessageType: "sms-send-failed",
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
      rilMessageType: "sms-sent",
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
    options.rilMessageType = "datacallerror";
    this.sendDOMMessage(options);
    return;
  }

  if (RILQUIRKS_V5_LEGACY) {
    
    
    this.readSetupDataCall_v5(options);
    this.currentDataCalls[options.cid] = options;
    options.rilMessageType = "datacallstatechange";
    this.sendDOMMessage(options);
    
    
    this.getDataCallList();
    return;
  }
  
  
  this[REQUEST_DATA_CALL_LIST](length, options);
};
RIL[REQUEST_SIM_IO] = function REQUEST_SIM_IO(length, options) {
  
  
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
RIL[REQUEST_SEND_USSD] = function REQUEST_SEND_USSD(length, options) {
  if (DEBUG) {
    debug("REQUEST_SEND_USSD " + JSON.stringify(options)); 
  }
  options.rilMessageType = "sendussd";
  options.success = options.rilRequestError == 0 ? true : false;
  options.errorMsg = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
  this.sendDOMMessage(options);
};
RIL[REQUEST_CANCEL_USSD] = function REQUEST_CANCEL_USSD(length, options) {
  if (DEBUG) {
    debug("REQUEST_CANCEL_USSD" + JSON.stringify(options));
  }
  options.rilMessageType = "cancelussd";
  options.success = options.rilRequestError == 0 ? true : false;
  options.errorMsg = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
  this.sendDOMMessage(options);
};
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
  datacall.rilMessageType = "datacallstatechange";
  this.sendDOMMessage(datacall);
};
RIL[REQUEST_QUERY_FACILITY_LOCK] = function REQUEST_QUERY_FACILITY_LOCK(length, options) {
  options.success = options.rilRequestError == 0;
  if (!options.success) {
    options.errorMsg = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
  }

  if (length) {
    options.enabled = Buf.readUint32List()[0] == 0 ? false : true;
  }
  this.sendDOMMessage(options);
};
RIL[REQUEST_SET_FACILITY_LOCK] = function REQUEST_SET_FACILITY_LOCK(length, options) {
  options.success = options.rilRequestError == 0;
  if (!options.success) {
    options.errorMsg = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
  }
  options.retryCount = length ? Buf.readUint32List()[0] : -1;
  this.sendDOMMessage(options);
};
RIL[REQUEST_CHANGE_BARRING_PASSWORD] = null;
RIL[REQUEST_QUERY_NETWORK_SELECTION_MODE] = function REQUEST_QUERY_NETWORK_SELECTION_MODE(length, options) {
  this._receivedNetworkInfo(NETWORK_INFO_NETWORK_SELECTION_MODE);

  if (options.rilRequestError) {
    options.error = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
    this.sendDOMMessage(options);
    return;
  }

  let mode = Buf.readUint32List();
  let selectionMode;

  switch (mode[0]) {
    case NETWORK_SELECTION_MODE_AUTOMATIC:
      selectionMode = GECKO_NETWORK_SELECTION_AUTOMATIC;
      break;
    case NETWORK_SELECTION_MODE_MANUAL:
      selectionMode = GECKO_NETWORK_SELECTION_MANUAL;
      break;
    default:
      selectionMode = GECKO_NETWORK_SELECTION_UNKNOWN;
      break;
  }

  if (this.networkSelectionMode != selectionMode) {
    this.networkSelectionMode = options.mode = selectionMode;
    options.rilMessageType = "networkselectionmodechange";
    this._sendNetworkInfoMessage(NETWORK_INFO_NETWORK_SELECTION_MODE, options);
  }
};
RIL[REQUEST_SET_NETWORK_SELECTION_AUTOMATIC] = function REQUEST_SET_NETWORK_SELECTION_AUTOMATIC(length, options) {
  if (options.rilRequestError) {
    options.error = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
    this.sendDOMMessage(options);
    return;
  }

  this.sendDOMMessage(options);
};
RIL[REQUEST_SET_NETWORK_SELECTION_MANUAL] = function REQUEST_SET_NETWORK_SELECTION_MANUAL(length, options) {
  if (options.rilRequestError) {
    options.error = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
    this.sendDOMMessage(options);
    return;
  }

  this.sendDOMMessage(options);
};
RIL[REQUEST_QUERY_AVAILABLE_NETWORKS] = function REQUEST_QUERY_AVAILABLE_NETWORKS(length, options) {
  if (options.rilRequestError) {
    options.error = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
    this.sendDOMMessage(options);
    return;
  }

  options.networks = this._processNetworks();
  this.sendDOMMessage(options);
};
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

RIL.readDataCall_v5 = function readDataCall_v5(options) {
  if (!options) {
    options = {};
  }
  options.cid = Buf.readUint32().toString();
  options.active = Buf.readUint32(); 
  options.type = Buf.readString();
  options.apn = Buf.readString();
  options.address = Buf.readString();
  return options;
};

RIL.readDataCall_v6 = function readDataCall_v6(options) {
  if (!options) {
    options = {};
  }
  options.status = Buf.readUint32();  
  options.suggestedRetryTime = Buf.readUint32();
  options.cid = Buf.readUint32().toString();
  options.active = Buf.readUint32();  
  options.type = Buf.readString();
  options.ifname = Buf.readString();
  options.ipaddr = Buf.readString();
  options.dns = Buf.readString();
  options.gw = Buf.readString();
  if (options.dns) {
    options.dns = options.dns.split(" ");
  }
  
  if (options.ipaddr) {
    options.ipaddr = options.ipaddr.split(" ")[0];
  }
  if (options.gw) {
    options.gw = options.gw.split(" ")[0];
  }
  options.ip = null;
  options.netmask = null;
  options.broadcast = null;
  if (options.ipaddr) {
    options.ip = options.ipaddr.split("/")[0];
    let ip_value = netHelpers.stringToIP(options.ip);
    let prefix_len = options.ipaddr.split("/")[1];
    let mask_value = netHelpers.makeMask(prefix_len);
    options.netmask = netHelpers.ipToString(mask_value);
    options.broadcast = netHelpers.ipToString((ip_value & mask_value) + ~mask_value);
  }
  return options;
};

RIL[REQUEST_DATA_CALL_LIST] = function REQUEST_DATA_CALL_LIST(length, options) {
  if (options.rilRequestError) {
    return;
  }

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

  let newDataCallOptions = null;
  if (options.rilRequestType == REQUEST_SETUP_DATA_CALL) {
    newDataCallOptions = options;
  }
  this._processDataCallList(datacalls, newDataCallOptions);
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
    return;
  }

  this.SMSC = Buf.readString();
};
RIL[REQUEST_SET_SMSC_ADDRESS] = null;
RIL[REQUEST_REPORT_SMS_MEMORY_STATUS] = null;
RIL[REQUEST_REPORT_STK_SERVICE_IS_RUNNING] = null;
RIL[UNSOLICITED_RESPONSE_RADIO_STATE_CHANGED] = function UNSOLICITED_RESPONSE_RADIO_STATE_CHANGED() {
  let radioState = Buf.readUint32();

  
  if (this._isInitialRadioState) {
    this._isInitialRadioState = false;
    if (radioState != RADIO_STATE_OFF) {
      this.setRadioPower({on: false});
      return;
    }
  }

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

  if ((this.radioState == GECKO_RADIOSTATE_UNAVAILABLE ||
       this.radioState == GECKO_RADIOSTATE_OFF) &&
       newState == GECKO_RADIOSTATE_READY) {
    
    if (cdma) {
      this.getDeviceIdentity();
    } else {
      this.getIMEI();
      this.getIMEISV();
    }
    this.getBasebandVersion();
  }

  this.radioState = newState;
  this.sendDOMMessage({
    rilMessageType: "radiostatechange",
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
  if (DEBUG) debug("Network state changed, re-requesting phone state and ICC status");
  this.getICCStatus();
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
RIL[UNSOLICITED_ON_USSD] = function UNSOLICITED_ON_USSD() {
  let [typeCode, message] = Buf.readStringList();
  if (DEBUG) {
    debug("On USSD. Type Code: " + typeCode + " Message: " + message);
  }
  
  if (!message || message == "") {
    return;
  }
  this.sendDOMMessage({rilMessageType: "ussdreceived",
                       message: message});
};
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

  this.sendDOMMessage({rilMessageType: "nitzTime",
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
  this[REQUEST_DATA_CALL_LIST](length, {rilRequestError: ERROR_SUCCESS});
};
RIL[UNSOLICITED_SUPP_SVC_NOTIFICATION] = null;

RIL[UNSOLICITED_STK_SESSION_END] = function UNSOLICITED_STK_SESSION_END() {
  this.sendDOMMessage({rilMessageType: "stksessionend"});
};
RIL[UNSOLICITED_STK_PROACTIVE_COMMAND] = function UNSOLICITED_STK_PROACTIVE_COMMAND() {
  this.processStkProactiveCommand();
};
RIL[UNSOLICITED_STK_EVENT_NOTIFY] = function UNSOLICITED_STK_EVENT_NOTIFY() {
  this.processStkProactiveCommand();
};
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

  


  bcdChars: "0123456789*#,;",
  semiOctetToBcdChar: function semiOctetToBcdChar(semiOctet) {
    if (semiOctet >= 14) {
      throw new RangeError();
    }

    return this.bcdChars.charAt(semiOctet);
  },

  







  readSwappedNibbleBcdNum: function readSwappedNibbleBcdNum(pairs) {
    let number = 0;
    for (let i = 0; i < pairs; i++) {
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

  







  readSwappedNibbleBcdString: function readSwappedNibbleBcdString(pairs) {
    let str = "";
    for (let i = 0; i < pairs; i++) {
      let nibbleH = this.readHexNibble();
      let nibbleL = this.readHexNibble();
      if (nibbleL == 0x0F) {
        break;
      }

      str += this.semiOctetToBcdChar(nibbleL);
      if (nibbleH != 0x0F) {
        str += this.semiOctetToBcdChar(nibbleH);
      }
    }

    return str;
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

  






  read8BitUnpackedToString: function read8BitUnpackedToString(numOctets) {
    let ret = "";
    let escapeFound = false;
    let i;
    const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
    const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];

    for(i = 0; i < numOctets; i++) {
      let octet = this.readHexOctet();
      if (octet == 0xff) {
        i++;
        break;
      }

      if (escapeFound) {
        escapeFound = false;
        if (octet == PDU_NL_EXTENDED_ESCAPE) {
          
          
          
          ret += " ";
        } else if (octet == PDU_NL_RESERVED_CONTROL) {
          
          
          
          ret += " ";
        } else {
          ret += langShiftTable[octet];
        }
      } else if (octet == PDU_NL_EXTENDED_ESCAPE) {
        escapeFound = true;
      } else {
        ret += langTable[octet];
      }
    }

    Buf.seekIncoming((numOctets - i) * PDU_HEX_OCTET_SIZE);
    return ret;
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

  








  readICCUCS2String: function readICCUCS2String(scheme, numOctets) {
    let str = "";
    switch (scheme) {
      




      case 0x80:
        let isOdd = numOctets % 2;
        let i;
        for (i = 0; i < numOctets - isOdd; i += 2) {
          let code = (this.readHexOctet() << 8) | this.readHexOctet();
          if (code == 0xffff) {
            i += 2;
            break;
          }
          str += String.fromCharCode(code);
        }

        
        Buf.seekIncoming((numOctets - i) * PDU_HEX_OCTET_SIZE);
        break;
      case 0x81: 
      case 0x82:
        






















        let len = this.readHexOctet();
        let offset, headerLen;
        if (scheme == 0x81) {
          offset = this.readHexOctet() << 7;
          headerLen = 2;
        } else {
          offset = (this.readHexOctet() << 8) | this.readHexOctet();
          headerLen = 3;
        }

        for (let i = 0; i < len; i++) {
          let ch = this.readHexOctet();
          if (ch & 0x80) {
            
            str += String.fromCharCode((ch & 0x7f) + offset);
          } else {
            
            let count = 0, gotUCS2 = 0;
            while ((i + count + 1 < len)) {
              count++;
              if (this.readHexOctet() & 0x80) {
                gotUCS2 = 1;
                break;
              };
            }
            
            
            Buf.seekIncoming(-1 * (count + 1) * PDU_HEX_OCTET_SIZE);
            str += this.read8BitUnpackedToString(count + 1 - gotUCS2);
            i += count - gotUCS2;
          }
        }

        
        Buf.seekIncoming((numOctets - len - headerLen) * PDU_HEX_OCTET_SIZE);
        break;
    }
    return str;
  },

  







  readUserDataHeader: function readUserDataHeader(msg) {
    










    let header = {
      length: 0,
      langIndex: PDU_NL_IDENTIFIER_DEFAULT,
      langShiftIndex: PDU_NL_IDENTIFIER_DEFAULT
    };

    header.length = this.readHexOctet();
    if (DEBUG) debug("Read UDH length: " + header.length);

    let dataAvailable = header.length;
    while (dataAvailable >= 2) {
      let id = this.readHexOctet();
      let length = this.readHexOctet();
      if (DEBUG) debug("Read UDH id: " + id + ", length: " + length);

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
        case PDU_IEI_APPLICATION_PORT_ADDRESSING_SCHEME_8BIT: {
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
        case PDU_IEI_APPLICATION_PORT_ADDRESSING_SCHEME_16BIT: {
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
        case PDU_IEI_SPECIAL_SMS_MESSAGE_INDICATION:
          let msgInd = this.readHexOctet() & 0xFF;
          let msgCount = this.readHexOctet();
          dataAvailable -= 2;


          






          let storeType = msgInd & PDU_MWI_STORE_TYPE_BIT;
          let mwi = msg.mwi;
          if (!mwi) {
            mwi = msg.mwi = {};
          }

          if (storeType == PDU_MWI_STORE_TYPE_STORE) {
            
            
            mwi.discard = false;
          } else if (mwi.discard === undefined) {
            
            
            mwi.discard = true;
          }

          mwi.msgCount = msgCount & 0xFF;
          mwi.active = mwi.msgCount > 0;

          if (DEBUG) debug("MWI in TP_UDH received: " + JSON.stringify(mwi));

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

    msg.header = header;
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

    
    let addr = this.readSwappedNibbleBcdString(len / 2);
    if (addr.length <= 0) {
      if (DEBUG) debug("PDU error: no number provided");
      return null;
    }
    if ((toa >> 4) == (PDU_TOA_INTERNATIONAL >> 4)) {
      addr = '+' + addr;
    }

    return addr;
  },
  
  













  readAlphaIdentifier: function readAlphaIdentifier(numOctets) {
    let temp;

    
    if ((temp = GsmPDUHelper.readHexOctet()) == 0x80 ||
         temp == 0x81 ||
         temp == 0x82) {
      numOctets--;
      return this.readICCUCS2String(temp, numOctets);
    } else {
      Buf.seekIncoming(-1 * PDU_HEX_OCTET_SIZE);
      return this.read8BitUnpackedToString(numOctets);
    }
  },

  




















  readDiallingNumber: function readDiallingNumber(len) {
    if (DEBUG) debug("PDU: Going to read Dialling number: " + len);

    
    let toa = this.readHexOctet();

    let number = this.readSwappedNibbleBcdString(len - 1).toString();
    if (number.length <= 0) {
      if (DEBUG) debug("PDU error: no number provided");
      return null;
    }
    if ((toa >> 4) == (PDU_TOA_INTERNATIONAL >> 4)) {
      number = '+' + number;
    }
    return number;
  },

  







  readProtocolIndicator: function readProtocolIndicator(msg) {
    
    
    msg.pid = this.readHexOctet();

    msg.epid = msg.pid;
    switch (msg.epid & 0xC0) {
      case 0x40:
        
        switch (msg.epid) {
          case PDU_PID_SHORT_MESSAGE_TYPE_0:
            return;
          case PDU_PID_RETURN_CALL_MESSAGE:
            
            
            let mwi = msg.mwi = {};

            
            mwi.active = true;
            mwi.discard = false;
            mwi.msgCount = GECKO_VOICEMAIL_MESSAGE_COUNT_UNKNOWN;
            if (DEBUG) debug("TP-PID got return call message: " + msg.sender);
            return;
        }
        break;
    }

    msg.epid = PDU_PID_DEFAULT;
  },

  







  readDataCodingScheme: function readDataCodingScheme(msg) {
    let dcs = this.readHexOctet();
    if (DEBUG) debug("PDU: read dcs: " + dcs);

    
    this.readMessageWaitingFromDCS(msg, dcs);

    
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

  readMessageWaitingFromDCS: function readMessageWaitingFromDCS(msg, dcs) {
    
    
    
    let codingGroup = dcs & PDU_DCS_CODING_GROUP_BITS;

    if (codingGroup == PDU_DCS_CODING_GROUP_7BITS_DISCARD ||
        codingGroup == PDU_DCS_CODING_GROUP_7BITS_STORE ||
        codingGroup == PDU_DCS_CODING_GROUP_16BITS_STORE) {

      
      let active = (dcs & PDU_DCS_MWI_ACTIVE_BITS) == PDU_DCS_MWI_ACTIVE_VALUE;

      
      switch (dcs & PDU_DCS_MWI_TYPE_BITS) {
        case PDU_DCS_MWI_TYPE_VOICEMAIL:
          let mwi = msg.mwi;
          if (!mwi) {
            mwi = msg.mwi = {};
          }

          mwi.active = active;
          mwi.discard = codingGroup == PDU_DCS_CODING_GROUP_7BITS_DISCARD;
          mwi.msgCount = active ? GECKO_VOICEMAIL_MESSAGE_COUNT_UNKNOWN : 0;

          if (DEBUG) {
            debug("MWI in DCS received for voicemail: " + JSON.stringify(mwi));
          }
          break;
        case PDU_DCS_MWI_TYPE_FAX:
          if (DEBUG) debug("MWI in DCS received for fax");
          break;
        case PDU_DCS_MWI_TYPE_EMAIL:
          if (DEBUG) debug("MWI in DCS received for email");
          break;
        default:
          if (DEBUG) debug("MWI in DCS received for \"other\"");
          break;
      }
    }
  },

  




  readTimestamp: function readTimestamp() {
    let year   = this.readSwappedNibbleBcdNum(1) + PDU_TIMESTAMP_YEAR_OFFSET;
    let month  = this.readSwappedNibbleBcdNum(1) - 1;
    let day    = this.readSwappedNibbleBcdNum(1);
    let hour   = this.readSwappedNibbleBcdNum(1);
    let minute = this.readSwappedNibbleBcdNum(1);
    let second = this.readSwappedNibbleBcdNum(1);
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
      this.readUserDataHeader(msg);

      if (msg.encoding == PDU_DCS_MSG_CODING_7BITS_ALPHABET) {
        let headerBits = (msg.header.length + 1) * 8;
        let headerSeptets = Math.ceil(headerBits / 7);

        length -= headerSeptets;
        paddingBits = headerSeptets * 7 - headerBits;
      } else {
        length -= (msg.header.length + 1);
      }
    }

    if (DEBUG) debug("After header, " + length + " septets left of user data");

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
      mwi:       null, 
      replace:  false, 
      header:    null, 
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
      
      msg.SMSC = this.readSwappedNibbleBcdString(smscLength - 1);
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
  },
};

let StkCommandParamsFactory = {
  createParam: function createParam(cmdDetails, ctlvs) {
    let param;
    switch (cmdDetails.typeOfCommand) {
      case STK_CMD_SET_UP_MENU:
      case STK_CMD_SELECT_ITEM:
        param = this.processSelectItem(cmdDetails, ctlvs);
        break;
      case STK_CMD_DISPLAY_TEXT:
        param = this.processDisplayText(cmdDetails, ctlvs);
        break;
      case STK_CMD_SET_UP_IDLE_MODE_TEXT:
        param = this.processSetUpIdleModeText(cmdDetails, ctlvs);
        break;
      case STK_CMD_GET_INKEY:
        param = this.processGetInkey(cmdDetails, ctlvs);
        break;
      case STK_CMD_GET_INPUT:
        param = this.processGetInput(cmdDetails, ctlvs);
        break;
      case STK_CMD_SEND_SS:
      case STK_CMD_SEND_USSD:
      case STK_CMD_SEND_SMS:
      case STK_CMD_SEND_DTMF:
        param = this.processEventNotify(cmdDetails, ctlvs);
        break;
      case STK_CMD_SET_UP_CALL:
        param = this.processSetupCall(cmdDetails, ctlvs);
        break;
      case STK_LAUNCH_BROWSER:
        param = this.processLaunchBrowser(cmdDetails, ctlvs);
        break;
      default:
        debug("unknown proactive command");
        break;
    }
    return param;
  },

  







  processSelectItem: function processSelectItem(cmdDetails, ctlvs) {
    let menu = {};

    let ctlv = StkProactiveCmdHelper.searchForTag(COMPREHENSIONTLV_TAG_ALPHA_ID, ctlvs);
    if (ctlv) {
      menu.title = ctlv.value.identifier;
    }

    menu.items = [];
    for (let i = 0; i < ctlvs.length; i++) {
      let ctlv = ctlvs[i];
      if (ctlv.tag == COMPREHENSIONTLV_TAG_ITEM) {
        menu.items.push(ctlv.value);
      }
    }

    if (menu.items.length == 0) {
      RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Menu: Required value missing : items");
    }

    ctlv = StkProactiveCmdHelper.searchForTag(COMPREHENSIONTLV_TAG_ITEM_ID, ctlvs);
    if (ctlv) {
      menu.defaultItem = ctlv.value.identifier - 1;
    }

    
    menu.presentationType = cmdDetails.commandQualifier & 0x03;

    
    if (cmdDetails.commandQualifier & 0x80) {
      menu.isHelpAvailable = true;
    }

    return menu;
  },

  processDisplayText: function processDisplayText(cmdDetails, ctlvs) {
    let textMsg = {};

    let ctlv = StkProactiveCmdHelper.searchForTag(COMPREHENSIONTLV_TAG_TEXT_STRING, ctlvs);
    if (!ctlv) {
      RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Display Text: Required value missing : Text String");
    }
    textMsg.text = ctlv.value.textString;

    ctlv = StkProactiveCmdHelper.searchForTag(COMPREHENSIONTLV_TAG_IMMEDIATE_RESPONSE, ctlvs);
    if (ctlv) {
      textMsg.responseNeeded = true;
    }

    
    if (cmdDetails.commandQualifier & 0x01) {
      textMsg.isHighPriority = true;
    }

    
    if (cmdDetails.commandQualifier & 0x80) {
      textMsg.userClear = true;
    }

    return textMsg;
  },

  processSetUpIdleModeText: function processSetUpIdleModeText(cmdDetails, ctlvs) {
    let textMsg = {};

    let ctlv = StkProactiveCmdHelper.searchForTag(COMPREHENSIONTLV_TAG_TEXT_STRING, ctlvs);
    if (!ctlv) {
      RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Set Up Idle Text: Required value missing : Text String");
    }
    textMsg.text = ctlv.value.textString;

    return textMsg;
  },

  processGetInkey: function processGetInkey(cmdDetails, ctlvs) {
    let input = {};

    let ctlv = StkProactiveCmdHelper.searchForTag(COMPREHENSIONTLV_TAG_TEXT_STRING, ctlvs);
    if (!ctlv) {
      RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Get InKey: Required value missing : Text String");
    }
    input.text = ctlv.value.textString;

    input.minLength = 1;
    input.maxLength = 1;

    
    if (cmdDetails.commandQualifier & 0x01) {
      input.isAlphabet = true;
    }

    
    if (cmdDetails.commandQualifier & 0x02) {
      input.isUCS2 = true;
    }

    
    
    if (cmdDetails.commandQualifier & 0x04) {
      input.isYesNoRequested = true;
    }

    
    if (cmdDetails.commandQualifier & 0x80) {
      input.isHelpAvailable = true;
    }

    return input;
  },

  processGetInput: function processGetInput(cmdDetails, ctlvs) {
    let input = {};

    let ctlv = StkProactiveCmdHelper.searchForTag(COMPREHENSIONTLV_TAG_TEXT_STRING, ctlvs);
    if (!ctlv) {
      RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Get Input: Required value missing : Text String");
    }
    input.text = ctlv.value.textString;

    ctlv = StkProactiveCmdHelper.searchForTag(COMPREHENSIONTLV_TAG_RESPONSE_LENGTH, ctlvs);
    if (ctlv) {
      input.minLength = ctlv.value.minLength;
      input.maxLength = ctlv.value.maxLength;
    }

    ctlv = StkProactiveCmdHelper.searchForTag(COMPREHENSIONTLV_TAG_DEFAULT_TEXT, ctlvs);
    if (ctlv) {
      input.defaultText = ctlv.value.textString;
    }

    
    if (cmdDetails.commandQualifier & 0x01) {
      input.isAlphabet = true;
    }

    
    if (cmdDetails.commandQualifier & 0x02) {
      input.isUCS2 = true;
    }

    
    if (cmdDetails.commandQualifier & 0x04) {
      input.hideInput = true;
    }

    
    if (cmdDetails.commandQualifier & 0x08) {
      input.isPacked = true;
    }

    
    if (cmdDetails.commandQualifier & 0x80) {
      input.isHelpAvailable = true;
    }

    return input;
  },

  processEventNotify: function processEventNotify(cmdDetails, ctlvs) {
    let textMsg = {};

    let ctlv = StkProactiveCmdHelper.searchForTag(COMPREHENSIONTLV_TAG_ALPHA_ID, ctlvs);
    if (!ctlv) {
      RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Event Notfiy: Required value missing : Alpha ID");
    }
    textMsg.text = ctlv.value.identifier;

    return textMsg;
  },

  processSetupCall: function processSetupCall(cmdDetails, ctlvs) {
    let call = {};

    for (let i = 0; i < ctlvs.length; i++) {
      let ctlv = ctlvs[i];
      if (ctlv.tag == COMPREHENSIONTLV_TAG_ALPHA_ID) {
        if (!call.confirmMessage) {
          call.confirmMessage = ctlv.value.identifier;
        } else {
          call.callMessge = ctlv.value.identifier;
          break;
        }
      }
    }

    let ctlv = StkProactiveCmdHelper.searchForTag(COMPREHENSIONTLV_TAG_ADDRESS, ctlvs);
    if (!ctlv) {
      RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Set Up Call: Required value missing : Adress");
    }
    call.address = ctlv.value.number;

    return call;
  },

  processLaunchBrowser: function processLaunchBrowser(cmdDetails, ctlvs) {
    let browser = {};

    let ctlv = StkProactiveCmdHelper.searchForTag(COMPREHENSIONTLV_TAG_URL, ctlvs);
    if (!ctlv) {
      RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Launch Browser: Required value missing : URL");
    }
    browser.url = ctlv.value.url;

    ctlv = StkProactiveCmdHelper.searchForTag(COMPREHENSIONTLV_TAG_ALPHA_ID, ctlvs)
    if (ctlv) {
      browser.confirmMessage = ctlv.value.identifier;
    }

    browser.mode = cmdDetails.commandQualifier & 0x03;

    return browser;
  }
};

let StkProactiveCmdHelper = {
  retrieve: function retrieve(tag, length) {
    switch (tag) {
      case COMPREHENSIONTLV_TAG_COMMAND_DETAILS:
        return this.retrieveCommandDetails(length);
      case COMPREHENSIONTLV_TAG_DEVICE_ID:
        return this.retrieveDeviceId(length);
      case COMPREHENSIONTLV_TAG_ALPHA_ID:
        return this.retrieveAlphaId(length);
      case COMPREHENSIONTLV_TAG_ADDRESS:
        return this.retrieveAddress(length);
      case COMPREHENSIONTLV_TAG_TEXT_STRING:
        return this.retrieveTextString(length);
      case COMPREHENSIONTLV_TAG_ITEM:
        return this.retrieveItem(length);
      case COMPREHENSIONTLV_TAG_ITEM_ID:
        return this.retrieveItemId(length);
      case COMPREHENSIONTLV_TAG_RESPONSE_LENGTH:
        return this.retrieveResponseLength(length);
      case COMPREHENSIONTLV_TAG_DEFAULT_TEXT:
        return this.retrieveDefaultText(length);
      case COMPREHENSIONTLV_TAG_IMMEDIATE_RESPONSE:
        return this.retrieveImmediaResponse(length);
      case COMPREHENSIONTLV_TAG_URL:
        return this.retrieveUrl(length);
      default:
        debug("StkProactiveCmdHelper: unknown tag " + tag.toString(16));
        Buf.seekIncoming(length * PDU_HEX_OCTET_SIZE);
        return null;
    }
  },

  









  retrieveCommandDetails: function retrieveCommandDetails(length) {
    let cmdDetails = {
      commandNumber: GsmPDUHelper.readHexOctet(),
      typeOfCommand: GsmPDUHelper.readHexOctet(),
      commandQualifier: GsmPDUHelper.readHexOctet()
    };
    return cmdDetails;
  },

  








  retrieveDeviceId: function retrieveDeviceId(length) {
    let deviceId = {
      sourceId: GsmPDUHelper.readHexOctet(),
      destinationId: GsmPDUHelper.readHexOctet()
    };
    return deviceId;
  },

  








  retrieveAlphaId: function retrieveAlphaId(length) {
    let alphaId = {
      identifier: GsmPDUHelper.readAlphaIdentifier(length)
    };
    return alphaId;
  },

  









  retrieveAddress: function retrieveAddress(length) {
    let address = {
      number : GsmPDUHelper.readDiallingNumber(length)
    };
    return address;
  },

  









  retrieveTextString: function retrieveTextString(length) {
    if (!length) {
      
      return null;
    }

    let text = {
      codingScheme: GsmPDUHelper.readHexOctet()
    };

    length--; 
    switch (text.codingScheme & 0x0f) {
      case STK_TEXT_CODING_GSM_7BIT_PACKED:
        text.textString = GsmPDUHelper.readSeptetsToString(length / 7, 0, 0, 0);
        break;
      case STK_TEXT_CODING_GSM_8BIT:
        text.textString = GsmPDUHelper.read8BitUnpackedToString(length);
        break;
      case STK_TEXT_CODING_UCS2:
        text.textString = GsmPDUHelper.readUCS2String(length);
        break;
    }
    return text;
  },

  









  retrieveItem: function retrieveItem(length) {
    let item = {
      identifier: GsmPDUHelper.readHexOctet(),
      text: GsmPDUHelper.readAlphaIdentifier(length - 1)
    };
    return item;
  },

  







  retrieveItemId: function retrieveItemId(length) {
    let itemId = {
      identifier: GsmPDUHelper.readHexOctet()
    };
    return itemId;
  },

  








  retrieveResponseLength: function retrieveResponseLength(length) {
    let rspLength = {
      minLength : GsmPDUHelper.readHexOctet(),
      maxLength : GsmPDUHelper.readHexOctet()
    };
    return rspLength;
  },

  




  retrieveDefaultText: function retrieveDefaultText(length) {
    return this.retrieveTextString(length);
  },

  






  retrieveImmediaResponse: function retrieveImmediaResponse(length) {
    return {};
  },

  








  retrieveUrl: function retrieveUrl(length) {
    let s = "";
    for (let i = 0; i < length; i++) {
      s += String.fromCharCode(GsmPDUHelper.readHexOctet());
    }
    return {url: s};
  },

  searchForTag: function searchForTag(tag, ctlvs) {
    for (let i = 0; i < ctlvs.length; i++) {
      let ctlv = ctlvs[i];
      if ((ctlv.tag & ~COMPREHENSIONTLV_FLAG_CR) == tag) {
        return ctlv;
      }
    }
    return null;
  },
};

let ComprehensionTlvHelper = {
  


  decode: function decode() {
    let hlen = 0; 
    let temp = GsmPDUHelper.readHexOctet();
    hlen++;

    
    let tag, tagValue, cr;
    switch (temp) {
      
      case 0x0: 
      case 0xff: 
      case 0x80: 
        RIL.sendStkTerminalResponse({
          resultCode: STK_RESULT_CMD_DATA_NOT_UNDERSTOOD});
        throw new Error("Invalid octet when parsing Comprehension TLV :" + temp);
        break;
      case 0x7f: 
        
        
        
        
        tag = (GsmPDUHelper.readHexOctet() << 8) | GsmPDUHelper.readHexOctet();
        hlen += 2;
        cr = (tag & 0x8000) != 0;
        tag &= ~0x8000;
        break;
      default: 
        tag = temp;
        
        
        
        cr = (tag & 0x80) != 0;
        tag &= ~0x80;
    }

    
    
    
    
    
    
    
    
    

    let length; 
    temp = GsmPDUHelper.readHexOctet();
    hlen++;
    if (temp < 0x80) {
      length = temp;
    } else if (temp == 0x81) {
      length = GsmPDUHelper.readHexOctet();
      hlen++;
      if (length < 0x80) {
        RIL.sendStkTerminalResponse({
          resultCode: STK_RESULT_CMD_DATA_NOT_UNDERSTOOD});
        throw new Error("Invalid length in Comprehension TLV :" + length);
      }
    } else if (temp == 0x82) {
      length = (GsmPDUHelper.readHexOctet() << 8) | GsmPDUHelper.readHexOctet();
      hlen += 2;
      if (lenth < 0x0100) {
         RIL.sendStkTerminalResponse({
          resultCode: STK_RESULT_CMD_DATA_NOT_UNDERSTOOD});
        throw new Error("Invalid length in 3-byte Comprehension TLV :" + length);
      }
    } else if (temp == 0x83) {
      length = (GsmPDUHelper.readHexOctet() << 16) |
               (GsmPDUHelper.readHexOctet() << 8)  |
                GsmPDUHelper.readHexOctet();
      hlen += 3;
      if (length < 0x010000) {
        RIL.sendStkTerminalResponse({
          resultCode: STK_RESULT_CMD_DATA_NOT_UNDERSTOOD});
        throw new Error("Invalid length in 4-byte Comprehension TLV :" + length);
      }
    } else {
      RIL.sendStkTerminalResponse({
        resultCode: STK_RESULT_CMD_DATA_NOT_UNDERSTOOD});
      throw new Error("Invalid octet in Comprehension TLV :" + length);
    }

    let ctlv = {
      tag: tag,
      length: length,
      value: StkProactiveCmdHelper.retrieve(tag, length),
      cr: cr,
      hlen: hlen
    };
    return ctlv;
  },

  decodeChunks: function decodeChunks(length) {
    let chunks = [];
    let index = 0;
    while (index < length) {
      let tlv = this.decode();
      chunks.push(tlv);
      index += tlv.length;
      index += tlv.hlen;
    }
    return chunks;
  }
};

let BerTlvHelper = {
  





  decode: function decode(dataLen) {
    
    let hlen = 0;
    let tag = GsmPDUHelper.readHexOctet();
    hlen++;

    
    
    
    let length;
    if (tag == BER_PROACTIVE_COMMAND_TAG) {
      let temp = GsmPDUHelper.readHexOctet();
      hlen++;
      if (temp < 0x80) {
        length = temp;
      } else if(temp == 0x81) {
        length = GsmPDUHelper.readHexOctet();
        if (length < 0x80) {
          RIL.sendStkTerminalResponse({
            resultCode: STK_RESULT_CMD_DATA_NOT_UNDERSTOOD});
          throw new Error("Invalid length " + length);
        }
      } else {
        RIL.sendStkTerminalResponse({
          resultCode: STK_RESULT_CMD_DATA_NOT_UNDERSTOOD});
        throw new Error("Invalid length octet " + temp);
      }
    } else {
      RIL.sendStkTerminalResponse({
        resultCode: STK_RESULT_CMD_DATA_NOT_UNDERSTOOD});
      throw new Error("Unknown BER tag");
    }

    
    if (dataLen - hlen < length) {
      RIL.sendStkTerminalResponse({
        resultCode: STK_RESULT_CMD_DATA_NOT_UNDERSTOOD});
      throw new Error("BerTlvHelper value length too long!!");
      return;
    }

    let ctlvs = ComprehensionTlvHelper.decodeChunks(length);
    let berTlv = {
      tag: tag,
      length: length,
      value: ctlvs
    };
    return berTlv;
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
