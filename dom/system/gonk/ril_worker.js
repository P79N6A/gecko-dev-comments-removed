





































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


const MMI_MATCH_GROUP_FULL_MMI = 1;
const MMI_MATCH_GROUP_MMI_PROCEDURE = 2;
const MMI_MATCH_GROUP_SERVICE_CODE = 3;
const MMI_MATCH_GROUP_SIA = 5;
const MMI_MATCH_GROUP_SIB = 7;
const MMI_MATCH_GROUP_SIC = 9;
const MMI_MATCH_GROUP_PWD_CONFIRM = 11;
const MMI_MATCH_GROUP_DIALING_NUMBER = 12;

const MMI_MAX_LENGTH_SHORT_CODE = 2;

const MMI_END_OF_USSD = "#";

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

    
    this.outgoingBufferCalSizeQueue = [];
  },

  











  startCalOutgoingSize: function startCalOutgoingSize(writeFunction) {
    let sizeInfo = {index: this.outgoingIndex,
                    write: writeFunction};

    
    writeFunction.call(0);

    
    sizeInfo.size = this.outgoingIndex - sizeInfo.index;

    
    this.outgoingBufferCalSizeQueue.push(sizeInfo);
  },

  


  stopCalOutgoingSize: function stopCalOutgoingSize() {
    let sizeInfo = this.outgoingBufferCalSizeQueue.pop();

    
    let currentOutgoingIndex = this.outgoingIndex;
    
    let writeSize = this.outgoingIndex - sizeInfo.index - sizeInfo.size;

    
    
    this.outgoingIndex = sizeInfo.index;
    sizeInfo.write(writeSize);

    
    this.outgoingIndex = currentOutgoingIndex;
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

  readUint8Array: function readUint8Array(length) {
    
    let last = this.currentParcelSize - this.readAvailable;
    last += (length - 1);
    this.ensureIncomingAvailable(last);

    let array = new Uint8Array(length);
    for (let i = 0; i < length; i++) {
      array[i] = this.readUint8Unchecked();
    }

    this.readAvailable -= length;
    return array;
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

  



  






  ensureOutgoingAvailable: function ensureOutgoingAvailable(index) {
    if (index >= this.OUTGOING_BUFFER_LENGTH) {
      this.growOutgoingBuffer(index + 1);
    }
  },

  writeUint8: function writeUint8(value) {
    this.ensureOutgoingAvailable(this.outgoingIndex);

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

  copyIncomingToOutgoing: function copyIncomingToOutgoing(length) {
    if (!length || (length < 0)) {
      return;
    }

    let translatedReadIndexEnd = this.currentParcelSize - this.readAvailable + length - 1;
    this.ensureIncomingAvailable(translatedReadIndexEnd);

    let translatedWriteIndexEnd = this.outgoingIndex + length - 1
    this.ensureOutgoingAvailable(translatedWriteIndexEnd);

    let newIncomingReadIndex = this.incomingReadIndex + length;
    if (newIncomingReadIndex < this.INCOMING_BUFFER_LENGTH) {
      
      this.outgoingBytes.set(this.incomingBytes.subarray(this.incomingReadIndex, newIncomingReadIndex),
                             this.outgoingIndex);
    } else {
      
      newIncomingReadIndex %= this.INCOMING_BUFFER_LENGTH;
      this.outgoingBytes.set(this.incomingBytes.subarray(this.incomingReadIndex, this.INCOMING_BUFFER_LENGTH),
                             this.outgoingIndex);
      if (newIncomingReadIndex) {
        let firstPartLength = this.INCOMING_BUFFER_LENGTH - this.incomingReadIndex;
        this.outgoingBytes.set(this.incomingBytes.subarray(0, newIncomingReadIndex),
                               this.outgoingIndex + firstPartLength);
      }
    }

    this.incomingReadIndex = newIncomingReadIndex;
    this.readAvailable -= length;
    this.outgoingIndex += length;
  },

  



  





  writeToIncoming: function writeToIncoming(incoming) {
    
    
    
    
    let minMustAvailableSize = incoming.length + this.readIncoming;
    if (minMustAvailableSize > this.INCOMING_BUFFER_LENGTH) {
      this.growIncomingBuffer(minMustAvailableSize);
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
      if (!options) {
        if (DEBUG) {
          debug("Suspicious uninvited request found: " + token + ". Ignored!");
        }
        return;
      }

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
    postRILMessage(CLIENT_ID, parcel);
    this.outgoingIndex = PARCEL_SIZE_SIZE;
  },

  simpleRequest: function simpleRequest(type, options) {
    this.newParcel(type, options);
    this.sendParcel();
  }
};









let RIL = {
  


  currentCalls: {},

  


  currentDataCalls: {},

  




  _receivedSmsSegmentsMap: {},

  


  _pendingSentSmsMap: {},

  



  preferredNetworkType: null,

  



  cellBroadcastConfigs: null,
  mergedCellBroadcastConfig: null,

  _receivedSmsCbPagesMap: {},

  initRILState: function initRILState() {
    


    this.radioState = GECKO_RADIOSTATE_UNAVAILABLE;
    this._isInitialRadioState = true;

    


    this._isCdma = false;

    



    this._waitingRadioTech = false;

    



    this.iccStatus = null;

    


    this.cardState = GECKO_CARDSTATE_UNKNOWN;

    


    this.IMEI = null;
    this.IMEISV = null;
    this.ESN = null;
    this.MEID = null;
    this.SMSC = null;

    


    this.iccInfoPrivate = {};

    


    this.iccInfo = {};

    


    this.cdmaHome = null;

    


    this.aid = null;

    


    this.appType = null,

    this.networkSelectionMode = null;

    this.voiceRegistrationState = {};
    this.dataRegistrationState = {};

    


    this.operator = null;

    


    this.basebandVersion = null;

    
    for each (let currentCall in this.currentCalls) {
      delete this.currentCalls[currentCall.callIndex];
      this._handleDisconnectedCall(currentCall);
    }

    
    for each (let datacall in this.currentDataCalls) {
      this.deactivateDataCall(datacall);
    }

    
    

    



    this._processingNetworkInfo = false;

    


    this._pendingNetworkInfo = {rilMessageType: "networkinfochanged"};

    


    this._muted = true;

    







    this._ussdSession = null;

   


    this._mmiRegExp = null;

    


    let cbmmi = this.cellBroadcastConfigs && this.cellBroadcastConfigs.MMI;
    this.cellBroadcastConfigs = {
      MMI: cbmmi || null
    };
  },

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
      case "nck":
        options.type = CARD_PERSOSUBSTATE_SIM_NETWORK;
        this.enterDepersonalization(options);
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
      Buf.writeString(options.aid || this.aid);
    }
    Buf.sendParcel();
  },

  







  enterICCPIN2: function enterICCPIN2(options) {
    Buf.newParcel(REQUEST_ENTER_SIM_PIN2, options);
    Buf.writeUint32(RILQUIRKS_V5_LEGACY ? 1 : 2);
    Buf.writeString(options.pin);
    if (!RILQUIRKS_V5_LEGACY) {
      Buf.writeString(options.aid || this.aid);
    }
    Buf.sendParcel();
  },

  







  enterDepersonalization: function enterDepersonalization(options) {
    Buf.newParcel(REQUEST_ENTER_NETWORK_DEPERSONALIZATION_CODE, options);
    Buf.writeUint32(options.type);
    Buf.writeString(options.pin);
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
      switch (options.lockType) {
        case "pin":
          options.facility = ICC_CB_FACILITY_SIM;
          options.password = options.pin;
          break;
        case "fdn":
          options.facility = ICC_CB_FACILITY_FDN;
          options.password = options.pin2;
          break;
        default:
          options.errorMsg = "Unsupported Card Lock.";
          options.success = false;
          this.sendDOMMessage(options);
          return;
      }
      options.enabled = options.enabled;
      options.serviceClass = ICC_SERVICE_CLASS_VOICE |
                             ICC_SERVICE_CLASS_DATA  |
                             ICC_SERVICE_CLASS_FAX;
      this.setICCFacilityLock(options);
    }
  },

  









  changeICCPIN: function changeICCPIN(options) {
    Buf.newParcel(REQUEST_CHANGE_SIM_PIN, options);
    Buf.writeUint32(RILQUIRKS_V5_LEGACY ? 2 : 3);
    Buf.writeString(options.pin);
    Buf.writeString(options.newPin);
    if (!RILQUIRKS_V5_LEGACY) {
      Buf.writeString(options.aid || this.aid);
    }
    Buf.sendParcel();
  },

  









  changeICCPIN2: function changeICCPIN2(options) {
    Buf.newParcel(REQUEST_CHANGE_SIM_PIN2, options);
    Buf.writeUint32(RILQUIRKS_V5_LEGACY ? 2 : 3);
    Buf.writeString(options.pin);
    Buf.writeString(options.newPin);
    if (!RILQUIRKS_V5_LEGACY) {
      Buf.writeString(options.aid || this.aid);
    }
    Buf.sendParcel();
  },
  









   enterICCPUK: function enterICCPUK(options) {
     Buf.newParcel(REQUEST_ENTER_SIM_PUK, options);
     Buf.writeUint32(RILQUIRKS_V5_LEGACY ? 2 : 3);
     Buf.writeString(options.puk);
     Buf.writeString(options.newPin);
     if (!RILQUIRKS_V5_LEGACY) {
       Buf.writeString(options.aid || this.aid);
     }
     Buf.sendParcel();
   },

  









   enterICCPUK2: function enterICCPUK2(options) {
     Buf.newParcel(REQUEST_ENTER_SIM_PUK2, options);
     Buf.writeUint32(RILQUIRKS_V5_LEGACY ? 2 : 3);
     Buf.writeString(options.puk);
     Buf.writeString(options.newPin);
     if (!RILQUIRKS_V5_LEGACY) {
       Buf.writeString(options.aid || this.aid);
     }
     Buf.sendParcel();
   },

  


  iccGetCardLock: function iccGetCardLock(options) {
    switch (options.lockType) {
      case "pin":
        options.facility = ICC_CB_FACILITY_SIM;
        break;
      case "fdn":
        options.facility = ICC_CB_FACILITY_FDN;
        break;
      default:
        options.errorMsg = "Unsupported Card Lock.";
        options.success = false;
        this.sendDOMMessage(options);
        return;
    }

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
      Buf.writeString(options.aid || this.aid);
    }
    Buf.sendParcel();
  },

  













  setICCFacilityLock: function setICCFacilityLock(options) {
    Buf.newParcel(REQUEST_SET_FACILITY_LOCK, options);
    Buf.writeUint32(RILQUIRKS_V5_LEGACY ? 3 : 4);
    Buf.writeString(options.facility);
    Buf.writeString(options.enabled ? "1" : "0");
    Buf.writeString(options.password);
    Buf.writeString(options.serviceClass.toString());
    if (!RILQUIRKS_V5_LEGACY) {
      Buf.writeString(options.aid || this.aid);
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
    if (options.command == ICC_COMMAND_UPDATE_RECORD &&
        options.dataWriter) {
      options.dataWriter(options.p3);
    } else {
      Buf.writeString(null);
    }
    Buf.writeString(options.pin2 || null);
    if (!RILQUIRKS_V5_LEGACY) {
      Buf.writeString(options.aid || this.aid);
    }
    Buf.sendParcel();
  },

  





  getIMSI: function getIMSI(aid) {
    if (RILQUIRKS_V5_LEGACY) {
      Buf.simpleRequest(REQUEST_GET_IMSI);
      return;
    }
    let token = Buf.newParcel(REQUEST_GET_IMSI);
    Buf.writeUint32(1);
    Buf.writeString(aid || this.aid);
    Buf.sendParcel();
  },

  







  getICCContacts: function getICCContacts(options) {
    if (!this.appType) {
      options.rilMessageType = "icccontacts";
      options.errorMsg = GECKO_ERROR_REQUEST_NOT_SUPPORTED;
      this.sendDOMMessage(options);
    }

    ICCContactHelper.readICCContacts(
      this.appType,
      options.contactType,
      function onsuccess(contacts) {
        
        options.rilMessageType = "icccontacts";
        options.contacts = contacts;
        RIL.sendDOMMessage(options);
      }.bind(this),
      function onerror(errorMsg) {
        options.rilMessageType = "icccontacts";
        options.errorMsg = errorMsg;
        RIL.sendDOMMessage(options);
      }.bind(this));
  },

  






  updateICCContact: function updateICCContact(options) {
    let onsuccess = function onsuccess() {
      
      options.rilMessageType = "icccontactupdate";
      RIL.sendDOMMessage(options);
    }.bind(this);

    let onerror = function onerror(errorMsg) {
      options.rilMessageType = "icccontactupdate";
      options.errorMsg = errorMsg;
      RIL.sendDOMMessage(options);
    }.bind(this);

    if (!this.appType || !options.contact) {
      onerror(GECKO_ERROR_REQUEST_NOT_SUPPORTED);
      return;
    }

    
    
    if (options.contact.recordId) {
      ICCContactHelper.updateICCContact(
        this.appType, options.contactType, options.contact, onsuccess, onerror);
    } else {
      ICCContactHelper.addICCContact(
        this.appType, options.contactType, options.contact, onsuccess, onerror);
    }
  },

  





  setRadioPower: function setRadioPower(options) {
    Buf.newParcel(REQUEST_RADIO_POWER);
    Buf.writeUint32(1);
    Buf.writeUint32(options.on ? 1 : 0);
    Buf.sendParcel();
  },

  





  setCallWaiting: function setCallWaiting(options) {
    Buf.newParcel(REQUEST_SET_CALL_WAITING, options);
    Buf.writeUint32(2);
    Buf.writeUint32(options.enabled ? 1 : 0);
    Buf.writeUint32(ICC_SERVICE_CLASS_VOICE);
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

  getVoiceRadioTechnology: function getVoiceRadioTechnology() {
    Buf.simpleRequest(REQUEST_VOICE_RADIO_TECH);
  },

  getDataRegistrationState: function getDataRegistrationState() {
    Buf.simpleRequest(REQUEST_DATA_REGISTRATION_STATE);
  },

  getOperator: function getOperator() {
    Buf.simpleRequest(REQUEST_OPERATOR);
  },

  







  setPreferredNetworkType: function setPreferredNetworkType(options) {
    if (options) {
      this.preferredNetworkType = options.networkType;
    }
    if (this.preferredNetworkType == null) {
      return;
    }

    Buf.newParcel(REQUEST_SET_PREFERRED_NETWORK_TYPE, options);
    Buf.writeUint32(1);
    Buf.writeUint32(this.preferredNetworkType);
    Buf.sendParcel();
  },

  


  getPreferredNetworkType: function getPreferredNetworkType() {
    Buf.simpleRequest(REQUEST_GET_PREFERRED_NETWORK_TYPE);
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

  


  iccOpenChannel: function iccOpenChannel(options) {
    if (DEBUG) {
      debug("iccOpenChannel: " + JSON.stringify(options));
    }

    let token = Buf.newParcel(REQUEST_SIM_OPEN_CHANNEL, options);
    Buf.writeString(options.aid);
    Buf.sendParcel();
  },




  iccExchangeAPDU: function iccExchangeAPDU(options) {
    if (DEBUG) debug("iccExchangeAPDU: " + JSON.stringify(options));

    var cla = options.apdu.cla;
    var command = options.apdu.command;
    var channel = options.channel;
    var path = options.apdu.path;
    var data = options.apdu.data;
    var data2 = options.apdu.data2;
    if (path == null || path === undefined) {
      var path = "";
    }
    if (data == null || data === undefined) {
      var data = "";
    }
    if (data2 == null || data2 === undefined) {
      var data2 = "";
    }
    var p1 = options.apdu.p1;
    var p2 = options.apdu.p2;
    var p3 = options.apdu.p3; 

    Buf.newParcel(REQUEST_SIM_ACCESS_CHANNEL, options);
    Buf.writeUint32(cla);
    Buf.writeUint32(command);
    Buf.writeUint32(channel);
    Buf.writeString(path); 
    Buf.writeUint32(p1);
    Buf.writeUint32(p2);
    Buf.writeUint32(p3);
    Buf.writeString(data); 
    Buf.writeString(data2);

    Buf.sendParcel();
  },

  


  iccCloseChannel: function iccCloseChannel(options) {
    if (DEBUG) debug("iccCloseChannel: " + JSON.stringify(options));

    Buf.newParcel(REQUEST_SIM_CLOSE_CHANNEL, options);
    Buf.writeUint32(1);
    Buf.writeUint32(options.channel);
    Buf.sendParcel();
  },

  


  selectNetwork: function selectNetwork(options) {
    if (DEBUG) {
      debug("Setting manual network selection: " + options.mcc + options.mnc);
    }

    
    let mnc = options.mnc.toString();
    if (mnc.length == 1)
      mnc = "0" + mnc;
    let numeric = options.mcc.toString() + mnc;
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

  getIMEI: function getIMEI(options) {
    Buf.simpleRequest(REQUEST_GET_IMEI, options);
  },

  getIMEISV: function getIMEISV() {
    Buf.simpleRequest(REQUEST_GET_IMEISV);
  },

  getDeviceIdentity: function getDeviceIdentity() {
    Buf.simpleRequest(REQUEST_DEVICE_IDENTITY);
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
    options.langIndex = options.langIndex || PDU_NL_IDENTIFIER_DEFAULT;
    options.langShiftIndex = options.langShiftIndex || PDU_NL_IDENTIFIER_DEFAULT;

    if (!options.retryCount) {
      options.retryCount = 0;
    }

    if (!options.segmentSeq) {
      
      options.segmentSeq = 1;
      options.body = options.segments[0].body;
      options.encodedBodyLength = options.segments[0].encodedBodyLength;
    }

    Buf.newParcel(REQUEST_SEND_SMS, options);
    Buf.writeUint32(2);
    Buf.writeString(options.SMSC);
    GsmPDUHelper.writeMessage(options);
    Buf.sendParcel();
  },

  







  acknowledgeGsmSms: function acknowledgeGsmSms(success, cause) {
    let token = Buf.newParcel(REQUEST_SMS_ACKNOWLEDGE);
    Buf.writeUint32(2);
    Buf.writeUint32(success ? 1 : 0);
    Buf.writeUint32(cause);
    Buf.sendParcel();
  },

  





  ackSMS: function ackSMS(options) {
    if (options.result == PDU_FCS_RESERVED) {
      return;
    }
    if (this._isCdma) {
      this.acknowledgeCdmaSms(options.result == PDU_FCS_OK, options.result);
    } else {
      this.acknowledgeGsmSms(options.result == PDU_FCS_OK, options.result);
    }
  },

  







  acknowledgeCdmaSms: function acknowledgeCdmaSms(success, cause) {
    let token = Buf.newParcel(REQUEST_CDMA_SMS_ACKNOWLEDGE);
    Buf.writeUint32(success ? 0 : 1);
    Buf.writeUint32(cause);
    Buf.sendParcel();
  },

  setCellBroadcastSearchList: function setCellBroadcastSearchList(options) {
    try {
      let str = options.searchListStr;
      this.cellBroadcastConfigs.MMI = this._convertCellBroadcastSearchList(str);
    } catch (e) {
      if (DEBUG) {
        debug("Invalid Cell Broadcast search list: " + e);
      }
      options.rilRequestError = ERROR_GENERIC_FAILURE;
      this.sendDOMMessage(options);
      return;
    }

    this._mergeAllCellBroadcastConfigs();
  },

  updateCellBroadcastConfig: function updateCellBroadcastConfig() {
    let activate = (this.mergedCellBroadcastConfig != null)
                   && (this.mergedCellBroadcastConfig.length > 0);
    if (activate) {
      this.setGsmSmsBroadcastConfig(this.mergedCellBroadcastConfig);
    } else {
      
      this.setGsmSmsBroadcastActivation(false);
    }
  },

  setGsmSmsBroadcastConfig: function setGsmSmsBroadcastConfig(config) {
    Buf.newParcel(REQUEST_GSM_SET_BROADCAST_SMS_CONFIG);

    let numConfigs = config ? config.length / 2 : 0;
    Buf.writeUint32(numConfigs);
    for (let i = 0; i < config.length;) {
      Buf.writeUint32(config[i++]);
      Buf.writeUint32(config[i++]);
      Buf.writeUint32(0x00);
      Buf.writeUint32(0xFF);
      Buf.writeUint32(1);
    }

    Buf.sendParcel();
  },

  setGsmSmsBroadcastActivation: function setGsmSmsBroadcastActivation(activate) {
    Buf.newParcel(REQUEST_GSM_SMS_BROADCAST_ACTIVATION);
    Buf.writeUint32(1);
    Buf.writeUint32(activate ? 1 : 0);
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

  


  _parseMMI: function _parseMMI(mmiString) {
    if (!mmiString || !mmiString.length) {
      return null;
    }

    
    if (this._mmiRegExp == null) {
      
      
      
      
      
      
      
      
      
      let pattern = "((\\*[*#]?|##?)";

      
      
      
      pattern += "(\\d{2,3})";

      
      
      
      
      
      
      
      
      
      
      
      
      
      pattern += "(\\*([^*#]*)(\\*([^*#]*)(\\*([^*#]*)";

      
      
      pattern += "(\\*([^*#]*))?)?)?)?#)";

      
      pattern += "([^#]*)";

      this._mmiRegExp = new RegExp(pattern);
    }
    let matches = this._mmiRegExp.exec(mmiString);

    
    
    
    
    if (matches == null) {
      if (mmiString.charAt(mmiString.length - 1) == MMI_END_OF_USSD) {
        return {
          fullMMI: mmiString
        };
      }
      return null;
    }

    
    
    
    
    
    
    
    
    
    
    return {
      fullMMI: matches[MMI_MATCH_GROUP_FULL_MMI],
      procedure: matches[MMI_MATCH_GROUP_MMI_PROCEDURE],
      serviceCode: matches[MMI_MATCH_GROUP_SERVICE_CODE],
      sia: matches[MMI_MATCH_GROUP_SIA],
      sib: matches[MMI_MATCH_GROUP_SIB],
      sic: matches[MMI_MATCH_GROUP_SIC],
      pwd: matches[MMI_MATCH_GROUP_PWD_CONFIRM],
      dialNumber: matches[MMI_MATCH_GROUP_DIALING_NUMBER]
    };
  },

  sendMMI: function sendMMI(options) {
    if (DEBUG) {
      debug("SendMMI " + JSON.stringify(options));
    }
    let mmiString = options.mmi;
    let mmi = this._parseMMI(mmiString);

    let _sendMMIError = (function _sendMMIError(errorMsg) {
      options.rilMessageType = "sendMMI";
      options.errorMsg = errorMsg;
      this.sendDOMMessage(options);
    }).bind(this);

    function _isValidPINPUKRequest() {
      
      
      if (!mmi.procedure || mmi.procedure != MMI_PROCEDURE_REGISTRATION ) {
        _sendMMIError("WRONG_MMI_PROCEDURE");
        return;
      }

      if (!mmi.sia || !mmi.sia.length || !mmi.sib || !mmi.sib.length ||
          !mmi.sic || !mmi.sic.length) {
        _sendMMIError("MISSING_SUPPLEMENTARY_INFORMATION");
        return;
      }

      if (mmi.sib != mmi.sic) {
        _sendMMIError("NEW_PIN_MISMATCH");
        return;
      }

      return true;
    }

    if (mmi == null) {
      if (this._ussdSession) {
        options.ussd = mmiString;
        this.sendUSSD(options);
        return;
      }
      _sendMMIError("NO_VALID_MMI_STRING");
      return;
    }

    if (DEBUG) {
      debug("MMI " + JSON.stringify(mmi));
    }

    
    
    let sc = mmi.serviceCode;

    switch (sc) {
      
      case MMI_SC_CFU:
      case MMI_SC_CF_BUSY:
      case MMI_SC_CF_NO_REPLY:
      case MMI_SC_CF_NOT_REACHABLE:
      case MMI_SC_CF_ALL:
      case MMI_SC_CF_ALL_CONDITIONAL:
        
        
        
        
        options.action = MMI_PROC_TO_CF_ACTION[mmi.procedure];
        options.rilMessageType = "sendMMI";
        options.reason = MMI_SC_TO_CF_REASON[sc];
        options.number = mmi.sia;
        options.serviceClass = this._siToServiceClass(mmi.sib);
        if (options.action == CALL_FORWARD_ACTION_QUERY_STATUS) {
          this.queryCallForwardStatus(options);
          return;
        }

        options.rilMessageType = "setCallForward";
        options.isSendMMI = true;
        options.timeSeconds = mmi.sic;
        this.setCallForward(options);
        return;

      
      case MMI_SC_PIN:
        
        
        
        
        if (!_isValidPINPUKRequest()) {
          return;
        }

        options.rilRequestType = "sendMMI";
        options.pin = mmi.sia;
        options.newPin = mmi.sib;
        this.changeICCPIN(options);
        return;

      
      case MMI_SC_PIN2:
        
        
        
        
        if (!_isValidPINPUKRequest()) {
          return;
        }

        options.rilRequestType = "sendMMI";
        options.pin = mmi.sia;
        options.newPin = mmi.sib;
        this.changeICCPIN2(options);
        return;

      
      case MMI_SC_PUK:
        
        
        
        
        if (!_isValidPINPUKRequest()) {
          return;
        }

        options.rilRequestType = "sendMMI";
        options.puk = mmi.sia;
        options.newPin = mmi.sib;
        this.enterICCPUK(options);
        return;

      
      case MMI_SC_PUK2:
        
        
        
        
        if (!_isValidPINPUKRequest()) {
          return;
        }

        options.rilRequestType = "sendMMI";
        options.puk = mmi.sia;
        options.newPin = mmi.sib;
        this.enterICCPUK2(options);
        return;

      
      case MMI_SC_IMEI:
        
        if (this.IMEI == null) {
          this.getIMEI({mmi: true});
          return;
        }
        
        options.rilMessageType = "sendMMI";
        options.success = true;
        options.result = this.IMEI;
        this.sendDOMMessage(options);
        return;

      
      case MMI_SC_BAOC:
      case MMI_SC_BAOIC:
      case MMI_SC_BAOICxH:
      case MMI_SC_BAIC:
      case MMI_SC_BAICr:
      case MMI_SC_BA_ALL:
      case MMI_SC_BA_MO:
      case MMI_SC_BA_MT:
        _sendMMIError("CALL_BARRING_NOT_SUPPORTED_VIA_MMI");
        return;

      
      case MMI_SC_CALL_WAITING:
        _sendMMIError("CALL_WAITING_NOT_SUPPORTED_VIA_MMI");
        return;
    }

    
    
    if (mmi.fullMMI &&
        (mmiString.charAt(mmiString.length - 1) == MMI_END_OF_USSD)) {
      options.ussd = mmi.fullMMI;
      this.sendUSSD(options);
      return;
    }

    
    
    _sendMMIError("NOT_VALID_MMI_STRING");
  },

  






   sendUSSD: function sendUSSD(options) {
     Buf.newParcel(REQUEST_SEND_USSD, options);
     Buf.writeString(options.ussd);
     Buf.sendParcel();
   },

  


   cancelUSSD: function cancelUSSD(options) {
     Buf.simpleRequest(REQUEST_CANCEL_USSD, options);
   },

  









  queryCallForwardStatus: function queryCallForwardStatus(options) {
    Buf.newParcel(REQUEST_QUERY_CALL_FORWARD_STATUS, options);
    Buf.writeUint32(CALL_FORWARD_ACTION_QUERY_STATUS);
    Buf.writeUint32(options.reason);
    Buf.writeUint32(options.serviceClass);
    Buf.writeUint32(this._toaFromString(options.number));
    Buf.writeString(options.number);
    Buf.writeUint32(0);
    Buf.sendParcel();
  },

  













  setCallForward: function setCallForward(options) {
    Buf.newParcel(REQUEST_SET_CALL_FORWARD, options);
    Buf.writeUint32(options.action);
    Buf.writeUint32(options.reason);
    Buf.writeUint32(options.serviceClass);
    Buf.writeUint32(this._toaFromString(options.number));
    Buf.writeString(options.number);
    Buf.writeUint32(options.timeSeconds);
    Buf.sendParcel();
  },

  





  stkHandleCallSetup: function stkHandleCallSetup(options) {
     Buf.newParcel(REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM, options);
     Buf.writeUint32(1);
     Buf.writeUint32(options.hasConfirmed ? 1 : 0);
     Buf.sendParcel();
  },

  




  sendStkTerminalProfile: function sendStkTerminalProfile(profile) {
    Buf.newParcel(REQUEST_STK_SET_PROFILE);
    Buf.writeUint32(profile.length * 2);
    for (let i = 0; i < profile.length; i++) {
      GsmPDUHelper.writeHexOctet(profile[i]);
    }
    Buf.writeUint32(0);
    Buf.sendParcel();
  },

  












  sendStkTerminalResponse: function sendStkTerminalResponse(response) {
    if (response.hasConfirmed !== undefined) {
      this.stkHandleCallSetup(response);
      return;
    }

    let token = Buf.newParcel(REQUEST_STK_SEND_TERMINAL_RESPONSE);
    let command = response.command;

    
    Buf.startCalOutgoingSize(function(size) {
      
      Buf.writeUint32(size / 2);
    });

    
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_COMMAND_DETAILS |
                               COMPREHENSIONTLV_FLAG_CR);
    GsmPDUHelper.writeHexOctet(3);
    if (response.command) {
      GsmPDUHelper.writeHexOctet(command.commandNumber);
      GsmPDUHelper.writeHexOctet(command.typeOfCommand);
      GsmPDUHelper.writeHexOctet(command.commandQualifier);
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

    
    if (response.itemIdentifier != null) {
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

        
        Buf.startCalOutgoingSize(function(size) {
          
          GsmPDUHelper.writeHexOctet(size / 4);
        });

        let coding = command.options.isUCS2 ?
                       STK_TEXT_CODING_UCS2 :
                       (command.options.isPacked ?
                          STK_TEXT_CODING_GSM_7BIT_PACKED :
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
            for (let i = 0; i < text.length; i++) {
              GsmPDUHelper.writeHexOctet(text.charCodeAt(i));
            }
            break;
        }

        
        Buf.stopCalOutgoingSize();
      }
    }

    
    if (response.localInfo) {
      let localInfo = response.localInfo;

      
      if (localInfo.locationInfo) {
        ComprehensionTlvHelper.writeLocationInfoTlv(localInfo.locationInfo);
      }

      
      if (localInfo.imei != null) {
        let imei = localInfo.imei;
        if(imei.length == 15) {
          imei = imei + "0";
        }

        GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_IMEI);
        GsmPDUHelper.writeHexOctet(8);
        for (let i = 0; i < imei.length / 2; i++) {
          GsmPDUHelper.writeHexOctet(parseInt(imei.substr(i * 2, 2), 16));
        }
      }

      
      if (localInfo.date != null) {
        ComprehensionTlvHelper.writeDateTimeZoneTlv(localInfo.date);
      }

      
      if (localInfo.language) {
        ComprehensionTlvHelper.writeLanguageTlv(localInfo.language);
      }
    }

    
    if (response.timer) {
      let timer = response.timer;

      if (timer.timerId) {
        GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_TIMER_IDENTIFIER);
        GsmPDUHelper.writeHexOctet(1);
        GsmPDUHelper.writeHexOctet(timer.timerId);
      }

      if (timer.timerValue) {
        ComprehensionTlvHelper.writeTimerValueTlv(timer.timerValue, false);
      }
    }

    
    Buf.stopCalOutgoingSize();

    Buf.writeUint32(0);
    Buf.sendParcel();
  },

  





  sendStkMenuSelection: function sendStkMenuSelection(command) {
    command.tag = BER_MENU_SELECTION_TAG;
    command.deviceId = {
      sourceId :STK_DEVICE_ID_KEYPAD,
      destinationId: STK_DEVICE_ID_SIM
    };
    this.sendICCEnvelopeCommand(command);
  },

  




  sendStkTimerExpiration: function sendStkTimerExpiration(command) {
    command.tag = BER_TIMER_EXPIRATION_TAG;
    command.deviceId = {
      sourceId: STK_DEVICE_ID_ME,
      destinationId: STK_DEVICE_ID_SIM
    };
    command.timerId = command.timer.timerId;
    command.timerValue = command.timer.timerValue;
    this.sendICCEnvelopeCommand(command);
  },

  



  sendStkEventDownload: function sendStkEventDownload(command) {
    command.tag = BER_EVENT_DOWNLOAD_TAG;
    command.eventList = command.event.eventType;
    switch (command.eventList) {
      case STK_EVENT_TYPE_LOCATION_STATUS:
        command.deviceId = {
          sourceId :STK_DEVICE_ID_ME,
          destinationId: STK_DEVICE_ID_SIM
        };
        command.locationStatus = command.event.locationStatus;
        
        if (command.locationStatus == STK_SERVICE_STATE_NORMAL) {
          command.locationInfo = command.event.locationInfo;
        }
        break;
      case STK_EVENT_TYPE_MT_CALL:
        command.deviceId = {
          sourceId: STK_DEVICE_ID_NETWORK,
          destinationId: STK_DEVICE_ID_SIM
        };
        command.transactionId = 0;
        command.address = command.event.number;
        break;
      case STK_EVENT_TYPE_CALL_DISCONNECTED:
        command.cause = command.event.error;
      case STK_EVENT_TYPE_CALL_CONNECTED:  
        command.deviceId = {
          sourceId: (command.event.isIssuedByRemote ?
                     STK_DEVICE_ID_NETWORK : STK_DEVICE_ID_ME),
          destinationId: STK_DEVICE_ID_SIM
        };
        command.transactionId = 0;
        break;
      case STK_EVENT_TYPE_IDLE_SCREEN_AVAILABLE:
        command.deviceId = {
          sourceId: STK_DEVICE_ID_DISPLAY,
          destinationId: STK_DEVICE_ID_SIM
        };
        break;
      case STK_EVENT_TYPE_LANGUAGE_SELECTION:
        command.deviceId = {
          sourceId: STK_DEVICE_ID_ME,
          destinationId: STK_DEVICE_ID_SIM
        };
        command.language = command.event.language;
        break;
    }
    this.sendICCEnvelopeCommand(command);
  },

  















  sendICCEnvelopeCommand: function sendICCEnvelopeCommand(options) {
    if (DEBUG) {
      debug("Stk Envelope " + JSON.stringify(options));
    }
    let token = Buf.newParcel(REQUEST_STK_SEND_ENVELOPE_COMMAND);

    
    Buf.startCalOutgoingSize(function(size) {
      
      Buf.writeUint32(size / 2);
    });

    
    GsmPDUHelper.writeHexOctet(options.tag);
    
    Buf.startCalOutgoingSize(function(size) {
      
      GsmPDUHelper.writeHexOctet(size / 4);
    });

    
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_DEVICE_ID |
                               COMPREHENSIONTLV_FLAG_CR);
    GsmPDUHelper.writeHexOctet(2);
    GsmPDUHelper.writeHexOctet(options.deviceId.sourceId);
    GsmPDUHelper.writeHexOctet(options.deviceId.destinationId);

    
    if (options.itemIdentifier != null) {
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

    
    if (options.eventList != null) {
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_EVENT_LIST |
                                 COMPREHENSIONTLV_FLAG_CR);
      GsmPDUHelper.writeHexOctet(1);
      GsmPDUHelper.writeHexOctet(options.eventList);
    }

    
    if (options.locationStatus != null) {
      let len = options.locationStatus.length;
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_LOCATION_STATUS |
                                 COMPREHENSIONTLV_FLAG_CR);
      GsmPDUHelper.writeHexOctet(1);
      GsmPDUHelper.writeHexOctet(options.locationStatus);
    }

    
    if (options.locationInfo) {
      ComprehensionTlvHelper.writeLocationInfoTlv(options.locationInfo);
    }

    
    if (options.transactionId != null) {
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_TRANSACTION_ID |
                                 COMPREHENSIONTLV_FLAG_CR);
      GsmPDUHelper.writeHexOctet(1);
      GsmPDUHelper.writeHexOctet(options.transactionId);
    }

    
    if (options.address) {
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_ADDRESS |
                                 COMPREHENSIONTLV_FLAG_CR);
      ComprehensionTlvHelper.writeLength(
        Math.ceil(options.address.length/2) + 1 
      );
      GsmPDUHelper.writeDiallingNumber(options.address);
    }

    
    if (options.cause != null) {
      ComprehensionTlvHelper.writeCauseTlv(options.cause);
    }

    
    if (options.timerId != null) {
        GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_TIMER_IDENTIFIER |
                                   COMPREHENSIONTLV_FLAG_CR);
        GsmPDUHelper.writeHexOctet(1);
        GsmPDUHelper.writeHexOctet(options.timerId);
    }

    
    if (options.timerValue != null) {
        ComprehensionTlvHelper.writeTimerValueTlv(options.timerValue, true);
    }

    
    if (options.language) {
      ComprehensionTlvHelper.writeLanguageTlv(options.language);
    }

    
    Buf.stopCalOutgoingSize();

    
    Buf.stopCalOutgoingSize();

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

  


  reportStkServiceIsRunning: function reportStkServiceIsRunning() {
    Buf.simpleRequest(REQUEST_REPORT_STK_SERVICE_IS_RUNNING);
  },

  


  _processICCStatus: function _processICCStatus(iccStatus) {
    this.iccStatus = iccStatus;
    let newCardState;

    if ((!iccStatus) || (iccStatus.cardState == CARD_STATE_ABSENT)) {
      switch (this.radioState) {
        case GECKO_RADIOSTATE_UNAVAILABLE:
          newCardState = GECKO_CARDSTATE_UNKNOWN;
          break;
        case GECKO_RADIOSTATE_OFF:
          newCardState = GECKO_CARDSTATE_NOT_READY;
          break;
        case GECKO_RADIOSTATE_READY:
          if (DEBUG) {
            debug("ICC absent");
          }
          newCardState = GECKO_CARDSTATE_ABSENT;
          break;
      }
      if (newCardState == this.cardState) {
        return;
      }
      this.cardState = newCardState;
      this.sendDOMMessage({rilMessageType: "cardstatechange",
                           cardState: this.cardState});
      return;
    }

    let index = this._isCdma ? iccStatus.cdmaSubscriptionAppIndex :
                               iccStatus.gsmUmtsSubscriptionAppIndex;
    let app = iccStatus.apps[index];
    if (iccStatus.cardState == CARD_STATE_ERROR || !app) {
      if (this.cardState == GECKO_CARDSTATE_UNKNOWN) {
        this.operator = null;
        return;
      }
      this.operator = null;
      this.cardState = GECKO_CARDSTATE_UNKNOWN;
      this.sendDOMMessage({rilMessageType: "cardstatechange",
                           cardState: this.cardState});
      return;
    }
    
    this.aid = app.aid;
    this.appType = app.app_type;

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
        newCardState = GECKO_CARDSTATE_UNKNOWN;
    }

    if (this.cardState == newCardState) {
      return;
    }

    
    this.requestNetworkInfo();
    this.getSignalStrength();
    if (newCardState == GECKO_CARDSTATE_READY) {
      
      
      if (this.appType == CARD_APPTYPE_SIM) {
        ICCRecordHelper.getICCPhase();
        ICCRecordHelper.fetchICCRecords();
      } else if (this.appType == CARD_APPTYPE_USIM) {
        this.sendStkTerminalProfile(STK_SUPPORTED_TERMINAL_PROFILE);
        ICCRecordHelper.fetchICCRecords();
      } else if (this.appType == CARD_APPTYPE_RUIM) {
        this.sendStkTerminalProfile(STK_SUPPORTED_TERMINAL_PROFILE);
        RuimRecordHelper.fetchRuimRecords();
      }
      this.reportStkServiceIsRunning();
    }

    this.cardState = newCardState;
    this.sendDOMMessage({rilMessageType: "cardstatechange",
                         cardState: this.cardState});
  },

   


  _processEnterAndChangeICCResponses: function _processEnterAndChangeICCResponses(length, options) {
    options.success = options.rilRequestError == 0;
    if (!options.success) {
      options.errorMsg = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
    }
    options.retryCount = length ? Buf.readUint32List()[0] : -1;
    this.sendDOMMessage(options);
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

    if (this._isCdma) {
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

      this.operator.mcc = 0;
      this.operator.mnc = 0;

      if (networkTuple) {
        try {
          this._processNetworkTuple(networkTuple, this.operator);
        } catch (e) {
          debug("Error processing operator tuple: " + e);
        }
      } else {
        
        
        
        if (DEBUG) {
          debug("Operator is currently unregistered");
        }
      }

      let networkName;
      
      if (this.voiceRegistrationState.cell &&
          this.voiceRegistrationState.cell.gsmLocationAreaCode != -1) {
        networkName = ICCUtilsHelper.getNetworkNameFromICC(
          this.operator.mcc,
          this.operator.mnc,
          this.voiceRegistrationState.cell.gsmLocationAreaCode);
      }

      if (networkName) {
        if (DEBUG) {
          debug("Operator names will be overriden: " +
                "longName = " + networkName.fullName + ", " +
                "shortName = " + networkName.shortName);
        }

        this.operator.longName = networkName.fullName;
        this.operator.shortName = networkName.shortName;
      } else {
        this.operator.longName = longName;
        this.operator.shortName = shortName;
      }

      if (ICCUtilsHelper.updateDisplayCondition()) {
        ICCUtilsHelper.handleICCInfoChange();
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

      if (!newCall) {
        
        
        delete this.currentCalls[currentCall.callIndex];
        this.getFailCauseCode(currentCall);
        continue;
      }

      
      if (newCall.state == currentCall.state) {
        continue;
      }

      
      if (newCall.state == CALL_STATE_INCOMING &&
          currentCall.state == CALL_STATE_WAITING) {
        
        
        currentCall.state = newCall.state;
        continue;
      }

      if (!currentCall.started && newCall.state == CALL_STATE_ACTIVE) {
        currentCall.started = new Date().getTime();
      }
      currentCall.state = newCall.state;
      this._handleChangedCallState(currentCall);
    }

    
    for each (let newCall in newCalls) {
      if (newCall.isVoice) {
        
        if (newCall.number &&
            newCall.toa == TOA_INTERNATIONAL &&
            newCall.number[0] != "+") {
          newCall.number = "+" + newCall.number;
        }
        if (newCall.state == CALL_STATE_INCOMING) {
          newCall.direction = 'incoming';
        } else if (newCall.state == CALL_STATE_DIALING) {
          newCall.direction = 'outgoing';
        }
        
        this.currentCalls[newCall.callIndex] = newCall;
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

  _sendDataCallError: function _sendDataCallError(message, errorCode) {
    message.rilMessageType = "datacallerror";
    if (errorCode == ERROR_GENERIC_FAILURE) {
      message.error = RIL_ERROR_TO_GECKO_ERROR[errorCode];
    } else {
      message.error = RIL_DATACALL_FAILCAUSE_TO_GECKO_DATACALL_ERROR[errorCode];
    }
    this.sendDOMMessage(message);
  },

  _processDataCallList: function _processDataCallList(datacalls, newDataCallOptions) {
    
    
    for each (let newDataCall in datacalls) {
      if (newDataCall.status != DATACALL_FAIL_NONE) {
        if (newDataCallOptions) {
          newDataCall.apn = newDataCallOptions.apn;
        }
        this._sendDataCallError(newDataCall, newDataCall.status);
      }
    }

    for each (let currentDataCall in this.currentDataCalls) {
      let updatedDataCall;
      if (datacalls) {
        updatedDataCall = datacalls[currentDataCall.cid];
        delete datacalls[currentDataCall.cid];
      }

      if (!updatedDataCall) {
        
        
        if (!newDataCallOptions) {
          currentDataCall.state = GECKO_NETWORK_STATE_DISCONNECTED;
          currentDataCall.rilMessageType = "datacallstatechange";
          this.sendDOMMessage(currentDataCall);
        }
        continue;
      }

      if (updatedDataCall && !updatedDataCall.ifname) {
        delete this.currentDataCalls[currentDataCall.cid];
        currentDataCall.state = GECKO_NETWORK_STATE_UNKNOWN;
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
      if (!newDataCall.ifname) {
        continue;
      }
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

  


  _processRadioTech: function _processRadioTech(radioTech) {
    let isCdma = true;
    this.radioTech = radioTech;

    switch(radioTech) {
    case NETWORK_CREG_TECH_GPRS:
    case NETWORK_CREG_TECH_EDGE:
    case NETWORK_CREG_TECH_UMTS:
    case NETWORK_CREG_TECH_HSDPA:
    case NETWORK_CREG_TECH_HSUPA:
    case NETWORK_CREG_TECH_HSPA:
    case NETWORK_CREG_TECH_LTE:
    case NETWORK_CREG_TECH_HSPAP:
    case NETWORK_CREG_TECH_GSM:
      isCdma = false;
    };

    if (DEBUG) {
      debug("Radio tech is set to: " + GECKO_RADIO_TECH[radioTech] +
            ", it is a " + (isCdma?"cdma":"gsm") + " technology");
    }

    
    
    
    if (this._waitingRadioTech || isCdma != this._isCdma) {
      this._isCdma = isCdma;
      this._waitingRadioTech = false;
      if (this._isCdma) {
        this.getDeviceIdentity();
      } else {
        this.getIMEI();
        this.getIMEISV();
      }
       this.getICCStatus();
    }
  },

  


  _toaFromString: function _toaFromString(number) {
    let toa = TOA_UNKNOWN;
    if (number && number.length > 0 && number[0] == '+') {
      toa = TOA_INTERNATIONAL;
    }
    return toa;
  },

  



  _siToServiceClass: function _siToServiceClass(si) {
    if (!si) {
      return ICC_SERVICE_CLASS_NONE;
    }

    let serviceCode = parseInt(si, 10);
    switch (serviceCode) {
      case 10:
        return ICC_SERVICE_CLASS_SMS + ICC_SERVICE_CLASS_FAX  + ICC_SERVICE_CLASS_VOICE;
      case 11:
        return ICC_SERVICE_CLASS_VOICE;
      case 12:
        return ICC_SERVICE_CLASS_SMS + ICC_SERVICE_CLASS_FAX;
      case 13:
        return ICC_SERVICE_CLASS_FAX;
      case 16:
        return ICC_SERVICE_CLASS_SMS;
      case 19:
        return ICC_SERVICE_CLASS_FAX + ICC_SERVICE_CLASS_VOICE;
      case 21:
        return ICC_SERVICE_CLASS_PAD + ICC_SERVICE_CLASS_DATA_ASYNC;
      case 22:
        return ICC_SERVICE_CLASS_PACKET + ICC_SERVICE_CLASS_DATA_SYNC;
      case 25:
        return ICC_SERVICE_CLASS_DATA_ASYNC;
      case 26:
        return ICC_SERVICE_CLASS_DATA_SYNC + SERVICE_CLASS_VOICE;
      case 99:
        return ICC_SERVICE_CLASS_PACKET;
      default:
        return ICC_SERVICE_CLASS_NONE;
    }
  },

  




  dataDownloadViaSMSPP: function dataDownloadViaSMSPP(message) {
    let options = {
      pid: message.pid,
      dcs: message.dcs,
      encoding: message.encoding,
    };
    Buf.newParcel(REQUEST_STK_SEND_ENVELOPE_WITH_STATUS, options);

    Buf.seekIncoming(-1 * (Buf.currentParcelSize - Buf.readAvailable
                           - 2 * UINT32_SIZE)); 
    let messageStringLength = Buf.readUint32(); 
    let smscLength = GsmPDUHelper.readHexOctet(); 
    let tpduLength = (messageStringLength / 2) - (smscLength + 1); 

    
    
    
    let berLen = 4 +
                 (smscLength ? (2 + smscLength) : 0) +
                 (tpduLength <= 127 ? 2 : 3) + tpduLength; 

    let parcelLength = (berLen <= 127 ? 2 : 3) + berLen; 
    Buf.writeUint32(parcelLength * 2); 

    
    GsmPDUHelper.writeHexOctet(BER_SMS_PP_DOWNLOAD_TAG);
    if (berLen > 127) {
      GsmPDUHelper.writeHexOctet(0x81);
    }
    GsmPDUHelper.writeHexOctet(berLen);

    
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_DEVICE_ID |
                               COMPREHENSIONTLV_FLAG_CR);
    GsmPDUHelper.writeHexOctet(0x02);
    GsmPDUHelper.writeHexOctet(STK_DEVICE_ID_NETWORK);
    GsmPDUHelper.writeHexOctet(STK_DEVICE_ID_SIM);

    
    if (smscLength) {
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_ADDRESS);
      GsmPDUHelper.writeHexOctet(smscLength);
      Buf.copyIncomingToOutgoing(PDU_HEX_OCTET_SIZE * smscLength);
    }

    
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_SMS_TPDU |
                               COMPREHENSIONTLV_FLAG_CR);
    if (tpduLength > 127) {
      GsmPDUHelper.writeHexOctet(0x81);
    }
    GsmPDUHelper.writeHexOctet(tpduLength);
    Buf.copyIncomingToOutgoing(PDU_HEX_OCTET_SIZE * tpduLength);

    
    Buf.writeStringDelimiter(0);

    Buf.sendParcel();
  },

  








  acknowledgeIncomingGsmSmsWithPDU: function acknowledgeIncomingGsmSmsWithPDU(success, responsePduLen, options) {
    Buf.newParcel(REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU);

    
    Buf.writeUint32(2);

    
    Buf.writeString(success ? "1" : "0");

    
    Buf.writeUint32(2 * (responsePduLen + (success ? 5 : 6))); 
    
    GsmPDUHelper.writeHexOctet(PDU_MTI_SMS_DELIVER);
    if (!success) {
      
      GsmPDUHelper.writeHexOctet(PDU_FCS_USIM_DATA_DOWNLOAD_ERROR);
    }
    
    GsmPDUHelper.writeHexOctet(PDU_PI_USER_DATA_LENGTH |
                               PDU_PI_DATA_CODING_SCHEME |
                               PDU_PI_PROTOCOL_IDENTIFIER);
    
    GsmPDUHelper.writeHexOctet(options.pid);
    
    GsmPDUHelper.writeHexOctet(options.dcs);
    
    if (options.encoding == PDU_DCS_MSG_CODING_7BITS_ALPHABET) {
      GsmPDUHelper.writeHexOctet(Math.floor(responsePduLen * 8 / 7));
    } else {
      GsmPDUHelper.writeHexOctet(responsePduLen);
    }
    
    Buf.copyIncomingToOutgoing(PDU_HEX_OCTET_SIZE * responsePduLen);
    
    Buf.writeStringDelimiter(0);

    Buf.sendParcel();
  },

  


  writeSmsToSIM: function writeSmsToSIM(message) {
    Buf.newParcel(REQUEST_WRITE_SMS_TO_SIM);

    
    Buf.writeUint32(EFSMS_STATUS_FREE);

    Buf.seekIncoming(-1 * (Buf.currentParcelSize - Buf.readAvailable
                           - 2 * UINT32_SIZE)); 
    let messageStringLength = Buf.readUint32(); 
    let smscLength = GsmPDUHelper.readHexOctet(); 
    let pduLength = (messageStringLength / 2) - (smscLength + 1); 

    
    if (smscLength > 0) {
      Buf.seekIncoming(smscLength * PDU_HEX_OCTET_SIZE);
    }
    
    Buf.writeUint32(2 * pduLength); 
    if (pduLength) {
      Buf.copyIncomingToOutgoing(PDU_HEX_OCTET_SIZE * pduLength);
    }
    
    Buf.writeStringDelimiter(0);

    
    
    Buf.writeUint32(2 * (smscLength + 1)); 
    
    GsmPDUHelper.writeHexOctet(smscLength);
    
    if (smscLength) {
      Buf.seekIncoming(-1 * (Buf.currentParcelSize - Buf.readAvailable
                             - 2 * UINT32_SIZE 
                             - 2 * PDU_HEX_OCTET_SIZE)); 
      Buf.copyIncomingToOutgoing(PDU_HEX_OCTET_SIZE * smscLength);
    }
    
    Buf.writeStringDelimiter(0);

    Buf.sendParcel();
  },

  







  _processSmsMultipart: function _processSmsMultipart(message) {
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
      message.result = PDU_FCS_OK;
      if (message.messageClass == GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_2]) {
        
        
        message.result = PDU_FCS_RESERVED;
      }
      message.rilMessageType = "sms-received";
      this.sendDOMMessage(message);

      
      
      return MOZ_FCS_WAIT_FOR_EXPLICIT_ACK;
    }

    return PDU_FCS_OK;
  },

  







  _processSmsStatusReport: function _processSmsStatusReport(length) {
    let [message, result] = GsmPDUHelper.processReceivedSms(length);
    if (!message) {
      if (DEBUG) debug("invalid SMS-STATUS-REPORT");
      return PDU_FCS_UNSPECIFIED;
    }

    let options = this._pendingSentSmsMap[message.messageRef];
    if (!options) {
      if (DEBUG) debug("no pending SMS-SUBMIT message");
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
      if (DEBUG) debug("SMS-STATUS-REPORT: delivery still pending");
      return PDU_FCS_OK;
    }

    delete this._pendingSentSmsMap[message.messageRef];

    let deliveryStatus = ((status >>> 5) == 0x00)
                       ? GECKO_SMS_DELIVERY_STATUS_SUCCESS
                       : GECKO_SMS_DELIVERY_STATUS_ERROR;
    this.sendDOMMessage({
      rilMessageType: "sms-delivery",
      envelopeId: options.envelopeId,
      deliveryStatus: deliveryStatus
    });

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

  







  _processSmsSendResult: function _processSmsSendResult(length, options) {
    if (options.rilRequestError) {
      if (DEBUG) debug("_processSmsSendResult: rilRequestError = " + options.rilRequestError);
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

    if ((options.segmentMaxSeq > 1)
        && (options.segmentSeq < options.segmentMaxSeq)) {
      
      this._processSentSmsSegment(options);
    } else {
      
      if (options.requestStatusReport) {
        if (DEBUG) debug("waiting SMS-STATUS-REPORT for messageRef " + options.messageRef);
        this._pendingSentSmsMap[options.messageRef] = options;
      }

      this.sendDOMMessage({
        rilMessageType: "sms-sent",
        envelopeId: options.envelopeId,
      });
    }
  },

  _processReceivedSmsCbPage: function _processReceivedSmsCbPage(original) {
    if (original.numPages <= 1) {
      if (original.body) {
        original.fullBody = original.body;
        delete original.body;
      } else if (original.data) {
        original.fullData = original.data;
        delete original.data;
      }
      return original;
    }

    
    let hash = original.serial + ":" + this.iccInfo.mcc + ":"
               + this.iccInfo.mnc + ":";
    switch (original.geographicalScope) {
      case CB_GSM_GEOGRAPHICAL_SCOPE_CELL_WIDE_IMMEDIATE:
      case CB_GSM_GEOGRAPHICAL_SCOPE_CELL_WIDE:
        hash += this.voiceRegistrationState.cell.gsmLocationAreaCode + ":"
             + this.voiceRegistrationState.cell.gsmCellId;
        break;
      case CB_GSM_GEOGRAPHICAL_SCOPE_LOCATION_AREA_WIDE:
        hash += this.voiceRegistrationState.cell.gsmLocationAreaCode + ":";
        break;
      default:
        hash += ":";
        break;
    }

    let index = original.pageIndex;

    let options = this._receivedSmsCbPagesMap[hash];
    if (!options) {
      options = original;
      this._receivedSmsCbPagesMap[hash] = options;

      options.receivedPages = 0;
      options.pages = [];
    } else if (options.pages[index]) {
      
      if (DEBUG) {
        debug("Got duplicated page no." + index + " of a multipage SMSCB: "
              + JSON.stringify(original));
      }
      return null;
    }

    if (options.encoding == PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      options.pages[index] = original.data;
      delete original.data;
    } else {
      options.pages[index] = original.body;
      delete original.body;
    }
    options.receivedPages++;
    if (options.receivedPages < options.numPages) {
      if (DEBUG) {
        debug("Got page no." + index + " of a multipage SMSCB: "
              + JSON.stringify(options));
      }
      return null;
    }

    
    delete this._receivedSmsCbPagesMap[hash];

    
    if (options.encoding == PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      
      let fullDataLen = 0;
      for (let i = 1; i <= options.numPages; i++) {
        fullDataLen += options.pages[i].length;
      }

      options.fullData = new Uint8Array(fullDataLen);
      for (let d= 0, i = 1; i <= options.numPages; i++) {
        let data = options.pages[i];
        for (let j = 0; j < data.length; j++) {
          options.fullData[d++] = data[j];
        }
      }
    } else {
      options.fullBody = options.pages.join("");
    }

    if (DEBUG) {
      debug("Got full multipage SMSCB: " + JSON.stringify(options));
    }

    return options;
  },

  _mergeCellBroadcastConfigs: function _mergeCellBroadcastConfigs(list, from, to) {
    if (!list) {
      return [from, to];
    }

    for (let i = 0, f1, t1; i < list.length;) {
      f1 = list[i++];
      t1 = list[i++];
      if (to == f1) {
        
        list[i - 2] = from;
        return list;
      }

      if (to < f1) {
        
        if (i > 2) {
          
          return list.slice(0, i - 2).concat([from, to]).concat(list.slice(i - 2));
        } else {
          return [from, to].concat(list);
        }
      }

      if (from > t1) {
        
        continue;
      }

      
      

      let changed = false;
      if (from < f1) {
        
        
        list[i - 2] = from;
        changed = true;
      }

      if (to <= t1) {
        
        
        return list;
      }

      
      let j = i;
      for (let f2, t2; j < list.length;) {
        f2 = list[j++];
        t2 = list[j++];
        if (to > t2) {
          
          
          continue;
        }

        if (to < t2) {
          if (to < f2) {
            
            
            j -= 2;
          } else if (to < t2) {
            
            
            to = t2;
          }
        }

        break;
      }

      
      list[i - 1] = to;

      if (j != i) {
        
        let ret = list.slice(0, i);
        if (j < list.length) {
          ret = ret.concat(list.slice(j));
        }
        return ret;
      }

      return list;
    }

    
    list.push(from);
    list.push(to);

    return list;
  },

  


  _mergeAllCellBroadcastConfigs: function _mergeAllCellBroadcastConfigs() {
    if (!("CBMI" in this.cellBroadcastConfigs)
        || !("CBMIR" in this.cellBroadcastConfigs)
        || !("MMI" in this.cellBroadcastConfigs)) {
      if (DEBUG) {
        debug("cell broadcast configs not ready, waiting ...");
      }
      return;
    }
    if (DEBUG) {
      debug("Cell Broadcast search lists: " + JSON.stringify(this.cellBroadcastConfigs));
    }

    let list = null;
    for each (let ll in this.cellBroadcastConfigs) {
      if (ll == null) {
        continue;
      }

      for (let i = 0; i < ll.length; i += 2) {
        list = this._mergeCellBroadcastConfigs(list, ll[i], ll[i + 1]);
      }
    }

    if (DEBUG) {
      debug("Cell Broadcast search lists(merged): " + JSON.stringify(list));
    }
    this.mergedCellBroadcastConfig = list;
    this.updateCellBroadcastConfig();
  },

  



  _checkCellBroadcastMMISettable: function _checkCellBroadcastMMISettable(from, to) {
    if ((to <= from) || (from >= 65536) || (from < 0)) {
      return false;
    }

    for (let i = 0, f, t; i < CB_NON_MMI_SETTABLE_RANGES.length;) {
      f = CB_NON_MMI_SETTABLE_RANGES[i++];
      t = CB_NON_MMI_SETTABLE_RANGES[i++];
      if ((from < t) && (to > f)) {
        
        return false;
      }
    }

    return true;
  },

  


  _convertCellBroadcastSearchList: function _convertCellBroadcastSearchList(searchListStr) {
    let parts = searchListStr && searchListStr.split(",");
    if (!parts) {
      return null;
    }

    let list = null;
    let result, from, to;
    for (let range of parts) {
      
      
      result = range.match(/^(\d+)(?:-(\d+))?$/);
      if (!result) {
        throw "Invalid format";
      }

      from = parseInt(result[1]);
      to = (result[2] != null) ? parseInt(result[2]) + 1 : from + 1;
      if (!this._checkCellBroadcastMMISettable(from, to)) {
        throw "Invalid range";
      }

      if (list == null) {
        list = [];
      }
      list.push(from);
      list.push(to);
    }

    return list;
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

    
    if (cmdDetails.typeOfCommand == STK_CMD_MORE_TIME) {
      RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_OK});
      return;
    }

    cmdDetails.rilMessageType = "stkcommand";
    cmdDetails.options = StkCommandParamsFactory.createParam(cmdDetails, ctlvs);
    RIL.sendDOMMessage(cmdDetails);
  },

  


  sendDOMMessage: function sendDOMMessage(message) {
    postMessage(message);
  },

  





  handleParcel: function handleParcel(request_type, length, options) {
    let method = this[request_type];
    if (typeof method == "function") {
      if (DEBUG) debug("Handling parcel as " + method.name);
      method.call(this, length, options);
    }
  },

  setDebugEnabled: function setDebugEnabled(options) {
    DEBUG = DEBUG_WORKER || options.enabled;
  }
};

RIL.initRILState();

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
RIL[REQUEST_ENTER_NETWORK_DEPERSONALIZATION_CODE] =
  function REQUEST_ENTER_NETWORK_DEPERSONALIZATION_CODE(length, options) {
  this._processEnterAndChangeICCResponses(length, options);
};
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

  this.iccInfoPrivate.imsi = Buf.readString();
  if (DEBUG) {
    debug("IMSI: " + this.iccInfoPrivate.imsi);
  }

  options.rilMessageType = "iccimsi";
  options.imsi = this.iccInfoPrivate.imsi;
  this.sendDOMMessage(options);
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
    default:
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
  this._processSmsSendResult(length, options);
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
    
    this._sendDataCallError(options, options.rilRequestError);
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
  if (!length) {
    ICCIOHelper.processICCIOError(options);
    return;
  }

  
  
  options.sw1 = Buf.readUint32();
  options.sw2 = Buf.readUint32();
  if (options.sw1 != ICC_STATUS_NORMAL_ENDING) {
    ICCIOHelper.processICCIOError(options);
    return;
  }
  ICCIOHelper.processICCIO(options);
};
RIL[REQUEST_SEND_USSD] = function REQUEST_SEND_USSD(length, options) {
  if (DEBUG) {
    debug("REQUEST_SEND_USSD " + JSON.stringify(options));
  }
  options.success = this._ussdSession = options.rilRequestError == 0;
  options.errorMsg = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
  this.sendDOMMessage(options);
};
RIL[REQUEST_CANCEL_USSD] = function REQUEST_CANCEL_USSD(length, options) {
  if (DEBUG) {
    debug("REQUEST_CANCEL_USSD" + JSON.stringify(options));
  }
  options.success = options.rilRequestError == 0;
  this._ussdSession = !options.success;
  options.errorMsg = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
  this.sendDOMMessage(options);
};
RIL[REQUEST_GET_CLIR] = null;
RIL[REQUEST_SET_CLIR] = null;
RIL[REQUEST_QUERY_CALL_FORWARD_STATUS] =
  function REQUEST_QUERY_CALL_FORWARD_STATUS(length, options) {
    options.success = options.rilRequestError == 0;
    if (!options.success) {
      options.errorMsg = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
      this.sendDOMMessage(options);
      return;
    }

    let rulesLength = 0;
    if (length) {
      rulesLength = Buf.readUint32();
    }
    if (!rulesLength) {
      options.success = false;
      options.errorMsg =
        "Invalid rule length while querying call forwarding status.";
      this.sendDOMMessage(options);
      return;
    }
    let rules = new Array(rulesLength);
    for (let i = 0; i < rulesLength; i++) {
      let rule = {};
      rule.active       = Buf.readUint32() == 1; 
      rule.reason       = Buf.readUint32(); 
      rule.serviceClass = Buf.readUint32();
      rule.toa          = Buf.readUint32();
      rule.number       = Buf.readString();
      rule.timeSeconds  = Buf.readUint32();
      rules[i] = rule;
    }
    options.rules = rules;
    this.sendDOMMessage(options);
};
RIL[REQUEST_SET_CALL_FORWARD] =
  function REQUEST_SET_CALL_FORWARD(length, options) {
    options.success = options.rilRequestError == 0;
    if (!options.success) {
      options.errorMsg = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
    }
    this.sendDOMMessage(options);
};
RIL[REQUEST_QUERY_CALL_WAITING] = null;
RIL[REQUEST_SET_CALL_WAITING] = function REQUEST_SET_CALL_WAITING(length, options) {
  options.errorMsg = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
  this.sendDOMMessage(options);
};
RIL[REQUEST_SMS_ACKNOWLEDGE] = null;
RIL[REQUEST_GET_IMEI] = function REQUEST_GET_IMEI(length, options) {
  this.IMEI = Buf.readString();
  
  if (!options.mmi) {
    return;
  }

  options.rilMessageType = "sendMMI";
  options.success = options.rilRequestError == 0;
  options.errorMsg = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
  if ((!options.success || this.IMEI == null) && !options.errorMsg) {
    options.errorMsg = GECKO_ERROR_GENERIC_FAILURE;
  }
  options.result = this.IMEI;
  this.sendDOMMessage(options);
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
  datacall.state = GECKO_NETWORK_STATE_UNKNOWN;
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
RIL[REQUEST_SIM_OPEN_CHANNEL] = function REQUEST_SIM_OPEN_CHANNEL(length, options) {
  if (options.rilRequestError) {
    options.error = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
    this.sendDOMMessage(options);
    return;
  }

  options.channel = Buf.readUint32();
  if (DEBUG) debug("Setting channel number in options: " + options.channel);
  this.sendDOMMessage(options);
};
RIL[REQUEST_SIM_CLOSE_CHANNEL] = function REQUEST_SIM_CLOSE_CHANNEL(length, options) {
  if (options.rilRequestError) {
    options.error = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
    this.sendDOMMessage(options);
    return;
  }

  
  this.sendDOMMessage(options);
};
RIL[REQUEST_SIM_ACCESS_CHANNEL] = function REQUEST_SIM_ACCESS_CHANNEL(length, options) {
  if (options.rilRequestError) {
    options.error = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
    this.sendDOMMessage(options);
  }

  options.sw1 = Buf.readUint32();
  options.sw2 = Buf.readUint32();
  options.simResponse = Buf.readString();
  if (DEBUG) {
    debug("Setting return values for RIL[REQUEST_SIM_ACCESS_CHANNEL]: ["
          + options.sw1 + "," + options.sw2 + ", " + options.simResponse + "]");
  }
  this.sendDOMMessage(options);
};
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
RIL[REQUEST_WRITE_SMS_TO_SIM] = function REQUEST_WRITE_SMS_TO_SIM(length, options) {
  if (options.rilRequestError) {
    
    
    
    
    this.acknowledgeGsmSms(false, PDU_FCS_PROTOCOL_ERROR);
  } else {
    this.acknowledgeGsmSms(true, PDU_FCS_OK);
  }
};
RIL[REQUEST_DELETE_SMS_ON_SIM] = null;
RIL[REQUEST_SET_BAND_MODE] = null;
RIL[REQUEST_QUERY_AVAILABLE_BAND_MODE] = null;
RIL[REQUEST_STK_GET_PROFILE] = null;
RIL[REQUEST_STK_SET_PROFILE] = null;
RIL[REQUEST_STK_SEND_ENVELOPE_COMMAND] = null;
RIL[REQUEST_STK_SEND_TERMINAL_RESPONSE] = null;
RIL[REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM] = null;
RIL[REQUEST_EXPLICIT_CALL_TRANSFER] = null;
RIL[REQUEST_SET_PREFERRED_NETWORK_TYPE] = function REQUEST_SET_PREFERRED_NETWORK_TYPE(length, options) {
  if (options.networkType == null) {
    
    return;
  }

  this.sendDOMMessage({
    rilMessageType: "setPreferredNetworkType",
    networkType: options.networkType,
    success: options.rilRequestError == ERROR_SUCCESS
  });
};
RIL[REQUEST_GET_PREFERRED_NETWORK_TYPE] = function REQUEST_GET_PREFERRED_NETWORK_TYPE(length, options) {
  let networkType;
  if (!options.rilRequestError) {
    networkType = RIL_PREFERRED_NETWORK_TYPE_TO_GECKO.indexOf(GECKO_PREFERRED_NETWORK_TYPE_DEFAULT);
    let responseLen = Buf.readUint32(); 
    if (responseLen) {
      this.preferredNetworkType = networkType = Buf.readUint32();
    }
  }

  this.sendDOMMessage({
    rilMessageType: "getPreferredNetworkType",
    networkType: networkType,
    success: options.rilRequestError == ERROR_SUCCESS
  });
};
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
RIL[REQUEST_GSM_SET_BROADCAST_SMS_CONFIG] = function REQUEST_GSM_SET_BROADCAST_SMS_CONFIG(length, options) {
  if (options.rilRequestError == ERROR_SUCCESS) {
    this.setGsmSmsBroadcastActivation(true);
  }
};
RIL[REQUEST_GSM_SMS_BROADCAST_ACTIVATION] = null;
RIL[REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG] = null;
RIL[REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG] = null;
RIL[REQUEST_CDMA_SMS_BROADCAST_ACTIVATION] = null;
RIL[REQUEST_CDMA_SUBSCRIPTION] = null;
RIL[REQUEST_CDMA_WRITE_SMS_TO_RUIM] = null;
RIL[REQUEST_CDMA_DELETE_SMS_ON_RUIM] = null;
RIL[REQUEST_DEVICE_IDENTITY] = function REQUEST_DEVICE_IDENTITY(length, options) {
  if (options.rilRequestError) {
    return;
  }

  let result = Buf.readStringList();

  
  
  
  this.ESN = result[2];
  this.MEID = result[3];
};
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
RIL[REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU] = null;
RIL[REQUEST_STK_SEND_ENVELOPE_WITH_STATUS] = function REQUEST_STK_SEND_ENVELOPE_WITH_STATUS(length, options) {
  if (options.rilRequestError) {
    this.acknowledgeGsmSms(false, PDU_FCS_UNSPECIFIED);
    return;
  }

  let sw1 = Buf.readUint32();
  let sw2 = Buf.readUint32();
  if ((sw1 == ICC_STATUS_SAT_BUSY) && (sw2 == 0x00)) {
    this.acknowledgeGsmSms(false, PDU_FCS_USAT_BUSY);
    return;
  }

  let success = ((sw1 == ICC_STATUS_NORMAL_ENDING) && (sw2 == 0x00))
                || (sw1 == ICC_STATUS_NORMAL_ENDING_WITH_EXTRA);

  let messageStringLength = Buf.readUint32(); 
  let responsePduLen = messageStringLength / 2; 
  if (!responsePduLen) {
    this.acknowledgeGsmSms(success, success ? PDU_FCS_OK
                                         : PDU_FCS_USIM_DATA_DOWNLOAD_ERROR);
    return;
  }

  this.acknowledgeIncomingGsmSmsWithPDU(success, responsePduLen, options);
};
RIL[REQUEST_VOICE_RADIO_TECH] = function REQUEST_VOICE_RADIO_TECH(length, options) {
  if (options.rilRequestError) {
    if (DEBUG) {
      debug("Error when getting voice radio tech: " + options.rilRequestError);
    }
    return;
  }
  let radioTech = Buf.readUint32List();
  this._processRadioTech(radioTech[0]);
};
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

  switch (radioState) {
  case RADIO_STATE_SIM_READY:
  case RADIO_STATE_SIM_NOT_READY:
  case RADIO_STATE_SIM_LOCKED_OR_ABSENT:
    this._isCdma = false;
    this._waitingRadioTech = false;
    break;
  case RADIO_STATE_RUIM_READY:
  case RADIO_STATE_RUIM_NOT_READY:
  case RADIO_STATE_RUIM_LOCKED_OR_ABSENT:
  case RADIO_STATE_NV_READY:
  case RADIO_STATE_NV_NOT_READY:
    this._isCdma = true;
    this._waitingRadioTech = false;
    break;
  case RADIO_STATE_ON: 
    
    
    
    this._waitingRadioTech = true;
    this.getVoiceRadioTechnology();
    break;
  }

  if ((this.radioState == GECKO_RADIOSTATE_UNAVAILABLE ||
       this.radioState == GECKO_RADIOSTATE_OFF) &&
       newState == GECKO_RADIOSTATE_READY) {
    
    if (!this._waitingRadioTech) {
      if (this._isCdma) {
        this.getDeviceIdentity();
      } else {
        this.getIMEI();
        this.getIMEISV();
      }
    }
    this.getBasebandVersion();
    this.updateCellBroadcastConfig();
  }

  this.radioState = newState;
  this.sendDOMMessage({
    rilMessageType: "radiostatechange",
    radioState: newState
  });

  
  
  
  if (radioState == RADIO_STATE_UNAVAILABLE ||
      radioState == RADIO_STATE_OFF ||
      this._waitingRadioTech) {
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
  let [message, result] = GsmPDUHelper.processReceivedSms(length);

  if (message) {
    result = this._processSmsMultipart(message);
  }

  if (result == PDU_FCS_RESERVED || result == MOZ_FCS_WAIT_FOR_EXPLICIT_ACK) {
    return;
  }

  
  this.acknowledgeGsmSms(result == PDU_FCS_OK, result);
};
RIL[UNSOLICITED_RESPONSE_NEW_SMS_STATUS_REPORT] = function UNSOLICITED_RESPONSE_NEW_SMS_STATUS_REPORT(length) {
  let result = this._processSmsStatusReport(length);
  this.acknowledgeGsmSms(result == PDU_FCS_OK, result);
};
RIL[UNSOLICITED_RESPONSE_NEW_SMS_ON_SIM] = function UNSOLICITED_RESPONSE_NEW_SMS_ON_SIM(length) {
  let info = Buf.readUint32List();
  
};
RIL[UNSOLICITED_ON_USSD] = function UNSOLICITED_ON_USSD() {
  let [typeCode, message] = Buf.readStringList();
  if (DEBUG) {
    debug("On USSD. Type Code: " + typeCode + " Message: " + message);
  }

  this._ussdSession = (typeCode != "0" && typeCode != "2");

  this.sendDOMMessage({rilMessageType: "USSDReceived",
                       message: message,
                       sessionEnded: !this._ussdSession});
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

  let timeInMS = Date.UTC(year + PDU_TIMESTAMP_YEAR_OFFSET, month - 1, day,
                          hours, minutes, seconds);

  if (isNaN(timeInMS)) {
    if (DEBUG) debug("NITZ failed to convert date");
    return;
  }

  this.sendDOMMessage({rilMessageType: "nitzTime",
                       networkTimeInMS: timeInMS,
                       networkTimeZoneInMinutes: -(tz * 15),
                       networkDSTInMinutes: -(dst * 60),
                       receiveTimeInMS: now});
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
  let info = {rilMessageType: "callRing"};
  let isCDMA = false; 
  if (isCDMA) {
    info.isPresent = Buf.readUint32();
    info.signalType = Buf.readUint32();
    info.alertPitch = Buf.readUint32();
    info.signal = Buf.readUint32();
  }
  
  
  
  
  this.sendDOMMessage(info);
};
RIL[UNSOLICITED_RESPONSE_SIM_STATUS_CHANGED] = function UNSOLICITED_RESPONSE_SIM_STATUS_CHANGED() {
  this.getICCStatus();
};
RIL[UNSOLICITED_RESPONSE_CDMA_NEW_SMS] = function UNSOLICITED_RESPONSE_CDMA_NEW_SMS(length) {
  let [message, result] = CdmaPDUHelper.processReceivedSms(length);

  if (message) {
    result = this._processSmsMultipart(message);
  }

  if (result == PDU_FCS_RESERVED || result == MOZ_FCS_WAIT_FOR_EXPLICIT_ACK) {
    return;
  }

  
  this.acknowledgeCdmaSms(result == PDU_FCS_OK, result);
};
RIL[UNSOLICITED_RESPONSE_NEW_BROADCAST_SMS] = function UNSOLICITED_RESPONSE_NEW_BROADCAST_SMS(length) {
  let message;
  try {
    message = GsmPDUHelper.readCbMessage(Buf.readUint32());
  } catch (e) {
    if (DEBUG) {
      debug("Failed to parse Cell Broadcast message: " + JSON.stringify(e));
    }
    return;
  }

  message = this._processReceivedSmsCbPage(message);
  if (!message) {
    return;
  }

  message.rilMessageType = "cellbroadcast-received";
  this.sendDOMMessage(message);
};
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

  this.initRILState();

  this.setPreferredNetworkType();
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

  








  BCDToOctet: function BCDToOctet(bcd) {
    bcd = Math.abs(bcd);
    return ((bcd % 10) << 4) + (Math.floor(bcd / 10) % 10);
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

  






  writeSwappedNibbleBCDNum: function writeSwappedNibbleBCDNum(data) {
    data = data.toString();
    if (data.length % 2) {
      data = "0" + data;
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
      let c = message.charAt(i);
      let septet = langTable.indexOf(c);
      if (septet == PDU_NL_EXTENDED_ESCAPE) {
        continue;
      }

      if (septet >= 0) {
        data |= septet << dataBits;
        dataBits += 7;
      } else {
        septet = langShiftTable.indexOf(c);
        if (septet == -1) {
          throw new Error("'" + c + "' is not in 7 bit alphabet "
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

  






  writeStringTo8BitUnpacked: function writeStringTo8BitUnpacked(numOctets, str) {
    const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
    const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];

    
    
    let i, j;
    let len = str ? str.length : 0;
    for (i = 0, j = 0; i < len && j < numOctets; i++) {
      let c = str.charAt(i);
      let octet = langTable.indexOf(c);

      if (octet == -1) {
        
        if (j + 2 > numOctets) {
          break;
        }

        octet = langShiftTable.indexOf(c);
        if (octet == -1) {
          
          octet = langTable.indexOf(' ');
        }
        this.writeHexOctet(PDU_NL_EXTENDED_ESCAPE);
        j++;
      }
      this.writeHexOctet(octet);
      j++;
    }

    
    while (j++ < numOctets) {
      this.writeHexOctet(0xff);
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

    if (options.dcs == PDU_DCS_MSG_CODING_7BITS_ALPHABET) {
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
    let addr = "";

    if ((toa & 0xF0) == PDU_TOA_ALPHANUMERIC) {
      addr = this.readSeptetsToString(Math.floor(len * 4 / 7), 0,
          PDU_NL_IDENTIFIER_DEFAULT , PDU_NL_IDENTIFIER_DEFAULT );
      return addr;
    }
    addr = this.readSwappedNibbleBcdString(len / 2);
    if (addr.length <= 0) {
      if (DEBUG) debug("PDU error: no number provided");
      return null;
    }
    if ((toa & 0xF0) == (PDU_TOA_INTERNATIONAL)) {
      addr = '+' + addr;
    }

    return addr;
  },

  




  readAlphaIdDiallingNumber: function readAlphaIdDiallingNumber(recordSize) {
    let length = Buf.readUint32();

    let alphaLen = recordSize - ADN_FOOTER_SIZE_BYTES;
    let alphaId = this.readAlphaIdentifier(alphaLen);

    let number;
    let numLen = this.readHexOctet();
    if (numLen != 0xff) {
      if (numLen > ADN_MAX_BCD_NUMBER_BYTES) {
        throw new Error("invalid length of BCD number/SSC contents - " + numLen);
      }

      number = this.readDiallingNumber(numLen);
      Buf.seekIncoming((ADN_MAX_BCD_NUMBER_BYTES - numLen) * PDU_HEX_OCTET_SIZE);
    } else {
      Buf.seekIncoming(ADN_MAX_BCD_NUMBER_BYTES * PDU_HEX_OCTET_SIZE);
    }

    
    Buf.seekIncoming(2 * PDU_HEX_OCTET_SIZE);
    Buf.readStringDelimiter(length);

    let contact = null;
    if (alphaId || number) {
      contact = {alphaId: alphaId,
                 number: number};
    }
    return contact;
  },

  






  writeAlphaIdDiallingNumber: function writeAlphaIdDiallingNumber(recordSize,
                                                                  alphaId,
                                                                  number) {
    
    let length = recordSize * 2;
    Buf.writeUint32(length);

    let alphaLen = recordSize - ADN_FOOTER_SIZE_BYTES;
    this.writeAlphaIdentifier(alphaLen, alphaId);

    if (number) {
      let numStart = number[0] == "+" ? 1 : 0;
      let numDigits = number.length - numStart;
      if (numDigits > ADN_MAX_NUMBER_DIGITS) {
        number = number.substring(0, ADN_MAX_NUMBER_DIGITS + numStart);
        numDigits = number.length - numStart;
      }

      
      let numLen = Math.ceil(numDigits / 2) + 1;
      this.writeHexOctet(numLen);
      this.writeDiallingNumber(number);
      
      for (let i = 0; i < ADN_MAX_BCD_NUMBER_BYTES - numLen; i++) {
        this.writeHexOctet(0xff);
      }
    } else {
      
      for (let i = 0; i < ADN_MAX_BCD_NUMBER_BYTES + 1; i++) {
        this.writeHexOctet(0xff);
      }
    }

    
    this.writeHexOctet(0xff);
    this.writeHexOctet(0xff);
    Buf.writeStringDelimiter(length);
  },

  













  readAlphaIdentifier: function readAlphaIdentifier(numOctets) {
    if (numOctets === 0) {
      return "";
    }

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

  










  writeAlphaIdentifier: function writeAlphaIdentifier(numOctets, alphaId) {
    if (numOctets === 0) {
      return;
    }

    
    if (!alphaId || ICCUtilsHelper.isGsm8BitAlphabet(alphaId)) {
      this.writeStringTo8BitUnpacked(numOctets, alphaId);
    } else {
      
      this.writeHexOctet(0x80);
      numOctets--;
      
      if (alphaId.length * 2 > numOctets) {
        alphaId = alphaId.substring(0, Math.floor(numOctets / 2));
      }
      this.writeUCS2String(alphaId);
      for (let i = alphaId.length * 2; i < numOctets; i++) {
        this.writeHexOctet(0xff);
      }
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

  




  writeDiallingNumber: function writeDiallingNumber(number) {
    let toa = PDU_TOA_ISDN; 
    if (number[0] == '+') {
      toa = PDU_TOA_INTERNATIONAL | PDU_TOA_ISDN; 
      number = number.substring(1);
    }
    this.writeHexOctet(toa);
    this.writeSwappedNibbleBCD(number);
  },

  







  readProtocolIndicator: function readProtocolIndicator(msg) {
    
    
    msg.pid = this.readHexOctet();

    msg.epid = msg.pid;
    switch (msg.epid & 0xC0) {
      case 0x40:
        
        switch (msg.epid) {
          case PDU_PID_SHORT_MESSAGE_TYPE_0:
          case PDU_PID_ANSI_136_R_DATA:
          case PDU_PID_USIM_DATA_DOWNLOAD:
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
    if (DEBUG) debug("PDU: read SMS dcs: " + dcs);

    
    let messageClass = PDU_DCS_MSG_CLASS_NORMAL;
    
    let encoding = PDU_DCS_MSG_CODING_7BITS_ALPHABET;
    switch (dcs & PDU_DCS_CODING_GROUP_BITS) {
      case 0x40: 
      case 0x50:
      case 0x60:
      case 0x70:
        
      case 0x00: 
      case 0x10:
      case 0x20:
      case 0x30:
        if (dcs & 0x10) {
          messageClass = dcs & PDU_DCS_MSG_CLASS_BITS;
        }
        switch (dcs & 0x0C) {
          case 0x4:
            encoding = PDU_DCS_MSG_CODING_8BITS_ALPHABET;
            break;
          case 0x8:
            encoding = PDU_DCS_MSG_CODING_16BITS_ALPHABET;
            break;
        }
        break;

      case 0xE0: 
        encoding = PDU_DCS_MSG_CODING_16BITS_ALPHABET;
        
        
      case 0xC0: 
      case 0xD0: 
        
        let active = (dcs & PDU_DCS_MWI_ACTIVE_BITS) == PDU_DCS_MWI_ACTIVE_VALUE;

        
        switch (dcs & PDU_DCS_MWI_TYPE_BITS) {
          case PDU_DCS_MWI_TYPE_VOICEMAIL:
            let mwi = msg.mwi;
            if (!mwi) {
              mwi = msg.mwi = {};
            }

            mwi.active = active;
            mwi.discard = (dcs & PDU_DCS_CODING_GROUP_BITS) == 0xC0;
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
        break;

      case 0xF0: 
        if (dcs & 0x04) {
          encoding = PDU_DCS_MSG_CODING_8BITS_ALPHABET;
        }
        messageClass = dcs & PDU_DCS_MSG_CLASS_BITS;
        break;

      default:
        
        break;
    }

    msg.dcs = dcs;
    msg.encoding = encoding;
    msg.messageClass = GECKO_SMS_MESSAGE_CLASSES[messageClass];

    if (DEBUG) debug("PDU: message encoding is " + encoding + " bit.");
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

  




  writeTimestamp: function writeTimestamp(date) {
    this.writeSwappedNibbleBCDNum(date.getFullYear() - PDU_TIMESTAMP_YEAR_OFFSET);

    
    
    this.writeSwappedNibbleBCDNum(date.getMonth() + 1);
    this.writeSwappedNibbleBCDNum(date.getDate());
    this.writeSwappedNibbleBCDNum(date.getHours());
    this.writeSwappedNibbleBCDNum(date.getMinutes());
    this.writeSwappedNibbleBCDNum(date.getSeconds());

    
    
    
    
    
    
    
    let zone = date.getTimezoneOffset() / 15;
    let octet = this.BCDToOctet(zone);

    
    
    
    
    
    if (zone > 0) {
      octet = octet | 0x08;
    }
    this.writeHexOctet(octet);
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

  







  processReceivedSms: function processReceivedSms(length) {
    if (!length) {
      if (DEBUG) debug("Received empty SMS!");
      return [null, PDU_FCS_UNSPECIFIED];
    }

    
    
    let messageStringLength = Buf.readUint32();
    if (DEBUG) debug("Got new SMS, length " + messageStringLength);
    let message = this.readMessage();
    if (DEBUG) debug("Got new SMS: " + JSON.stringify(message));

    
    Buf.readStringDelimiter(length);

    
    if (!message) {
      return [null, PDU_FCS_UNSPECIFIED];
    }

    if (message.epid == PDU_PID_SHORT_MESSAGE_TYPE_0) {
      
      
      
      return [null, PDU_FCS_OK];
    }

    if (message.messageClass == GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_2]) {
      switch (message.epid) {
        case PDU_PID_ANSI_136_R_DATA:
        case PDU_PID_USIM_DATA_DOWNLOAD:
          if (ICCUtilsHelper.isICCServiceAvailable("DATA_DOWNLOAD_SMS_PP")) {
            
            
            
            
            RIL.dataDownloadViaSMSPP(message);

            
            
            return [null, PDU_FCS_RESERVED];
          }
          

          
          
          
        default:
          RIL.writeSmsToSIM(message);
          break;
      }
    }

    
    if ((message.messageClass != GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_0]) && !true) {
      
      
      
      

      if (message.messageClass == GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_2]) {
        
        
        return [null, PDU_FCS_MEMORY_CAPACITY_EXCEEDED];
      }

      return [null, PDU_FCS_UNSPECIFIED];
    }

    return [message, PDU_FCS_OK];
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

  







  readCbSerialNumber: function readCbSerialNumber(msg) {
    msg.serial = Buf.readUint8() << 8 | Buf.readUint8();
    msg.geographicalScope = (msg.serial >>> 14) & 0x03;
    msg.messageCode = (msg.serial >>> 4) & 0x03FF;
    msg.updateNumber = msg.serial & 0x0F;
  },

  







  readCbMessageIdentifier: function readCbMessageIdentifier(msg) {
    msg.messageId = Buf.readUint8() << 8 | Buf.readUint8();

    if ((msg.format != CB_FORMAT_ETWS)
        && (msg.messageId >= CB_GSM_MESSAGEID_ETWS_BEGIN)
        && (msg.messageId <= CB_GSM_MESSAGEID_ETWS_END)) {
      
      
      
      msg.etws = {
        emergencyUserAlert: msg.messageCode & 0x0200 ? true : false,
        popup:              msg.messageCode & 0x0100 ? true : false
      };

      let warningType = msg.messageId - CB_GSM_MESSAGEID_ETWS_BEGIN;
      if (warningType < CB_ETWS_WARNING_TYPE_NAMES.length) {
        msg.etws.warningType = warningType;
      }
    }
  },

  







  readCbDataCodingScheme: function readCbDataCodingScheme(msg) {
    let dcs = Buf.readUint8();
    if (DEBUG) debug("PDU: read CBS dcs: " + dcs);

    let language = null, hasLanguageIndicator = false;
    
    
    let encoding = PDU_DCS_MSG_CODING_7BITS_ALPHABET;
    let messageClass = PDU_DCS_MSG_CLASS_NORMAL;

    switch (dcs & PDU_DCS_CODING_GROUP_BITS) {
      case 0x00: 
        language = CB_DCS_LANG_GROUP_1[dcs & 0x0F];
        break;

      case 0x10: 
        switch (dcs & 0x0F) {
          case 0x00:
            hasLanguageIndicator = true;
            break;
          case 0x01:
            encoding = PDU_DCS_MSG_CODING_16BITS_ALPHABET;
            hasLanguageIndicator = true;
            break;
        }
        break;

      case 0x20: 
        language = CB_DCS_LANG_GROUP_2[dcs & 0x0F];
        break;

      case 0x40: 
      case 0x50:
      
      
      case 0x90: 
        encoding = (dcs & 0x0C);
        if (encoding == 0x0C) {
          encoding = PDU_DCS_MSG_CODING_7BITS_ALPHABET;
        }
        messageClass = (dcs & PDU_DCS_MSG_CLASS_BITS);
        break;

      case 0xF0:
        encoding = (dcs & 0x04) ? PDU_DCS_MSG_CODING_8BITS_ALPHABET
                                : PDU_DCS_MSG_CODING_7BITS_ALPHABET;
        switch(dcs & PDU_DCS_MSG_CLASS_BITS) {
          case 0x01: messageClass = PDU_DCS_MSG_CLASS_USER_1; break;
          case 0x02: messageClass = PDU_DCS_MSG_CLASS_USER_2; break;
          case 0x03: messageClass = PDU_DCS_MSG_CLASS_3; break;
        }
        break;

      case 0x30: 
      case 0x80: 
      case 0xA0: 
      case 0xB0:
      case 0xC0:
        break;

      default:
        throw new Error("Unsupported CBS data coding scheme: " + dcs);
    }

    msg.dcs = dcs;
    msg.encoding = encoding;
    msg.language = language;
    msg.messageClass = GECKO_SMS_MESSAGE_CLASSES[messageClass];
    msg.hasLanguageIndicator = hasLanguageIndicator;
  },

  







  readCbPageParameter: function readCbPageParameter(msg) {
    let octet = Buf.readUint8();
    msg.pageIndex = (octet >>> 4) & 0x0F;
    msg.numPages = octet & 0x0F;
    if (!msg.pageIndex || !msg.numPages) {
      
      
      
      msg.pageIndex = msg.numPages = 1;
    }
  },

  







  readCbWarningType: function readCbWarningType(msg) {
    let word = Buf.readUint8() << 8 | Buf.readUint8();
    msg.etws = {
      warningType:        (word >>> 9) & 0x7F,
      popup:              word & 0x80 ? true : false,
      emergencyUserAlert: word & 0x100 ? true : false
    };
  },

  









  readGsmCbData: function readGsmCbData(msg, length) {
    let bufAdapter = {
      readHexOctet: function readHexOctet() {
        return Buf.readUint8();
      }
    };

    msg.body = null;
    msg.data = null;
    switch (msg.encoding) {
      case PDU_DCS_MSG_CODING_7BITS_ALPHABET:
        msg.body = this.readSeptetsToString.call(bufAdapter,
                                                 (length * 8 / 7), 0,
                                                 PDU_NL_IDENTIFIER_DEFAULT,
                                                 PDU_NL_IDENTIFIER_DEFAULT);
        if (msg.hasLanguageIndicator) {
          msg.language = msg.body.substring(0, 2);
          msg.body = msg.body.substring(3);
        }
        break;

      case PDU_DCS_MSG_CODING_8BITS_ALPHABET:
        msg.data = Buf.readUint8Array(length);
        break;

      case PDU_DCS_MSG_CODING_16BITS_ALPHABET:
        if (msg.hasLanguageIndicator) {
          msg.language = this.readSeptetsToString.call(bufAdapter, 2, 0,
                                                       PDU_NL_IDENTIFIER_DEFAULT,
                                                       PDU_NL_IDENTIFIER_DEFAULT);
          length -= 2;
        }
        msg.body = this.readUCS2String.call(bufAdapter, length);
        break;
    }
  },

  





  readCbMessage: function readCbMessage(pduLength) {
    
    let msg = {
      
      serial:               null,                              
      updateNumber:         null,                              
      format:               null,                              
      dcs:                  0x0F,                              
      encoding:             PDU_DCS_MSG_CODING_7BITS_ALPHABET, 
      hasLanguageIndicator: false,                             
      data:                 null,                              
      body:                 null,                              
      pageIndex:            1,                                 
      numPages:             1,                                 

      
      geographicalScope:    null,                              
      messageCode:          null,                              
      messageId:            null,                              
      language:             null,                              
      fullBody:             null,                              
      fullData:             null,                              
      messageClass:         GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL], 
      etws:                 null                               
      




    };

    if (pduLength <= CB_MESSAGE_SIZE_ETWS) {
      msg.format = CB_FORMAT_ETWS;
      return this.readEtwsCbMessage(msg);
    }

    if (pduLength <= CB_MESSAGE_SIZE_GSM) {
      msg.format = CB_FORMAT_GSM;
      return this.readGsmCbMessage(msg, pduLength);
    }

    return null;
  },

  









  readGsmCbMessage: function readGsmCbMessage(msg, pduLength) {
    this.readCbSerialNumber(msg);
    this.readCbMessageIdentifier(msg);
    this.readCbDataCodingScheme(msg);
    this.readCbPageParameter(msg);

    
    this.readGsmCbData(msg, pduLength - 6);

    return msg;
  },

  







  readEtwsCbMessage: function readEtwsCbMessage(msg) {
    this.readCbSerialNumber(msg);
    this.readCbMessageIdentifier(msg);
    this.readCbWarningType(msg);

    
    
    

    return msg;
  },

  










  readNetworkName: function readNetworkName(len) {
    
    
    
    
    
    
    
    
    

    let codingInfo = GsmPDUHelper.readHexOctet();
    if (!(codingInfo & 0x80)) {
      return null;
    }

    let textEncoding = (codingInfo & 0x70) >> 4,
        shouldIncludeCountryInitials = !!(codingInfo & 0x08),
        spareBits = codingInfo & 0x07;
    let resultString;

    switch (textEncoding) {
    case 0:
      
      resultString = GsmPDUHelper.readSeptetsToString(
        ((len - 1) * 8 - spareBits) / 7, 0,
        PDU_NL_IDENTIFIER_DEFAULT,
        PDU_NL_IDENTIFIER_DEFAULT);
      break;
    case 1:
      
      resultString = this.readUCS2String(len - 1);
      break;
    default:
      
      return null;
    }

    
    
    return resultString;
  }
};




let BitBufferHelper = {
  readCache: 0,
  readCacheSize: 0,
  readBuffer: [],
  readIndex: 0,
  writeCache: 0,
  writeCacheSize: 0,
  writeBuffer: [],

  
  
  readBits: function readBits(length) {
    if (length <= 0 || length > 32) {
      return null;
    }

    if (length > this.readCacheSize) {
      let bytesToRead = Math.ceil((length - this.readCacheSize) / 8);
      for(let i = 0; i < bytesToRead; i++) {
        this.readCache = (this.readCache << 8) | (this.readBuffer[this.readIndex++] & 0xFF);
        this.readCacheSize += 8;
      }
    }

    let bitOffset = (this.readCacheSize - length),
        resultMask = (1 << length) - 1,
        result = 0;

    result = (this.readCache >> bitOffset) & resultMask;
    this.readCacheSize -= length;

    return result;
  },

  writeBits: function writeBits(value, length) {
    if (length <= 0 || length > 32) {
      return;
    }

    let totalLength = length + this.writeCacheSize;

    
    if (totalLength < 8) {
      let valueMask = (1 << length) - 1;
      this.writeCache = (this.writeCache << length) | (value & valueMask);
      this.writeCacheSize += length;
      return;
    }

    
    if (this.writeCacheSize) {
      let mergeLength = 8 - this.writeCacheSize,
          valueMask = (1 << mergeLength) - 1;

      this.writeCache = (this.writeCache << mergeLength) | ((value >> (length - mergeLength)) & valueMask);
      this.writeBuffer.push(this.writeCache & 0xFF);
      length -= mergeLength;
    }

    
    while (length >= 8) {
      length -= 8;
      this.writeBuffer.push((value >> length) & 0xFF);
    }

    
    this.writeCacheSize = length;
    this.writeCache = value & ((1 << length) - 1);

    return;
  },

  
  
  nextOctetAlign: function nextOctetAlign() {
    this.readCache = 0;
    this.readCacheSize = 0;
  },

  
  
  flushWithPadding: function flushWithPadding() {
    if (this.writeCacheSize) {
      this.writeBuffer.push(this.writeCache << (8 - this.writeCacheSize));
    }
    this.writeCache = 0;
    this.writeCacheSize = 0;
  },

  startWrite: function startWrite(dataBuffer) {
    this.writeBuffer = dataBuffer;
    this.writeCache = 0;
    this.writeCacheSize = 0;
  },

  startRead: function startRead(dataBuffer) {
    this.readBuffer = dataBuffer;
    this.readCache = 0;
    this.readCacheSize = 0;
    this.readIndex = 0;
  },

  getWriteBufferSize: function getWriteBufferSize() {
    return this.writeBuffer.length;
  },

  overwriteWriteBuffer: function overwriteWriteBuffer(position, data) {
    let writeLength = data.length;
    if (writeLength + position >= this.writeBuffer.length) {
      writeLength = this.writeBuffer.length - position;
    }
    for (let i = 0; i < writeLength; i++) {
      this.writeBuffer[i + position] = data[i];
    }
  }
};








let CdmaPDUHelper = {
  
  
  dtmfChars: ".1234567890*#...",

  























  writeMessage: function cdma_writeMessage(options) {
    if (DEBUG) {
      debug("cdma_writeMessage: " + JSON.stringify(options));
    }

    
    options.encoding = this.gsmDcsToCdmaEncoding(options.dcs);

    
    if (options.segmentMaxSeq > 1) {
      this.writeInt(PDU_CDMA_MSG_TELESERIVCIE_ID_WEMT);
    } else {
      this.writeInt(PDU_CDMA_MSG_TELESERIVCIE_ID_SMS);
    }

    this.writeInt(0);
    this.writeInt(PDU_CDMA_MSG_CATEGORY_UNSPEC);

    
    let addrInfo = this.encodeAddr(options.number);
    this.writeByte(addrInfo.digitMode);
    this.writeByte(addrInfo.numberMode);
    this.writeByte(addrInfo.numberType);
    this.writeByte(addrInfo.numberPlan);
    this.writeByte(addrInfo.address.length);
    for (let i = 0; i < addrInfo.address.length; i++) {
      this.writeByte(addrInfo.address[i]);
    }

    
    this.writeByte(0);  
    this.writeByte(0);  
    this.writeByte(0);  

    
    let encodeResult = this.encodeUserData(options);
    this.writeByte(encodeResult.length);
    for (let i = 0; i < encodeResult.length; i++) {
      this.writeByte(encodeResult[i]);
    }

    encodeResult = null;
  },

  


  writeInt: function writeInt(value) {
    Buf.writeUint32(value);
  },

  writeByte: function writeByte(value) {
    Buf.writeUint32(value & 0xFF);
  },

  


  gsmDcsToCdmaEncoding: function gsmDcsToCdmaEncoding(encoding) {
    switch (encoding) {
      case PDU_DCS_MSG_CODING_7BITS_ALPHABET:
        return PDU_CDMA_MSG_CODING_7BITS_ASCII;
      case PDU_DCS_MSG_CODING_8BITS_ALPHABET:
        return PDU_CDMA_MSG_CODING_OCTET;
      case PDU_DCS_MSG_CODING_16BITS_ALPHABET:
        return PDU_CDMA_MSG_CODING_UNICODE;
    }
  },

  







  encodeAddr: function cdma_encodeAddr(address) {
    let result = {};

    result.numberType = PDU_CDMA_MSG_ADDR_NUMBER_TYPE_UNKNOWN;
    result.numberPlan = PDU_CDMA_MSG_ADDR_NUMBER_TYPE_UNKNOWN;

    if (address[0] === '+') {
      address = address.substring(1);
    }

    
    result.digitMode = PDU_CDMA_MSG_ADDR_DIGIT_MODE_DTMF;
    result.numberMode = PDU_CDMA_MSG_ADDR_NUMBER_MODE_ANSI;

    result.address = [];
    for (let i = 0; i < address.length; i++) {
      let addrDigit = this.dtmfChars.indexOf(address.charAt(i));
      if (addrDigit < 0) {
        result.digitMode = PDU_CDMA_MSG_ADDR_DIGIT_MODE_ASCII;
        result.numberMode = PDU_CDMA_MSG_ADDR_NUMBER_MODE_ASCII;
        result.address = [];
        break;
      }
      result.address.push(addrDigit)
    }

    
    if (result.digitMode !== PDU_CDMA_MSG_ADDR_DIGIT_MODE_DTMF) {
      if (address.indexOf("@") !== -1) {
        result.numberType = PDU_CDMA_MSG_ADDR_NUMBER_TYPE_NATIONAL;
      }

      for (let i = 0; i < address.length; i++) {
        result.address.push(address.charCodeAt(i) & 0x7F);
      }
    }

    return result;
  },

  
























  encodeUserData: function cdma_encodeUserData(options) {
    let userDataBuffer = [];
    BitBufferHelper.startWrite(userDataBuffer);

    
    this.encodeUserDataMsgId(options);

    
    this.encodeUserDataMsg(options);

    return userDataBuffer;
  },

  




  encodeUserDataMsgId: function cdma_encodeUserDataMsgId(options) {
    BitBufferHelper.writeBits(PDU_CDMA_MSG_USERDATA_MSG_ID, 8);
    BitBufferHelper.writeBits(3, 8);
    BitBufferHelper.writeBits(PDU_CDMA_MSG_TYPE_SUBMIT, 4);
    BitBufferHelper.writeBits(1, 16); 
    if (options.segmentMaxSeq > 1) {
      BitBufferHelper.writeBits(1, 1);
    } else {
      BitBufferHelper.writeBits(0, 1);
    }

    BitBufferHelper.flushWithPadding();
  },

  




  encodeUserDataMsg: function cdma_encodeUserDataMsg(options) {
    BitBufferHelper.writeBits(PDU_CDMA_MSG_USERDATA_BODY, 8);
    
    BitBufferHelper.writeBits(0, 8);
    let lengthPosition = BitBufferHelper.getWriteBufferSize();

    BitBufferHelper.writeBits(options.encoding, 5);

    
    let msgBody = options.body,
        msgBodySize = (options.encoding === PDU_CDMA_MSG_CODING_7BITS_ASCII ?
                       options.encodedBodyLength :
                       msgBody.length);
    if (options.segmentMaxSeq > 1) {
      if (options.encoding === PDU_CDMA_MSG_CODING_7BITS_ASCII) {
          BitBufferHelper.writeBits(msgBodySize + 7, 8); 

          BitBufferHelper.writeBits(5, 8);  
          BitBufferHelper.writeBits(PDU_IEI_CONCATENATED_SHORT_MESSAGES_8BIT, 8);  
          BitBufferHelper.writeBits(3, 8);  
          BitBufferHelper.writeBits(options.segmentRef & 0xFF, 8);      
          BitBufferHelper.writeBits(options.segmentMaxSeq & 0xFF, 8);   
          BitBufferHelper.writeBits(options.segmentSeq & 0xFF, 8);      
          BitBufferHelper.writeBits(0, 1);  
        } else {
          if (options.encoding === PDU_CDMA_MSG_CODING_UNICODE) {
            BitBufferHelper.writeBits(msgBodySize + 3, 8); 
          } else {
            BitBufferHelper.writeBits(msgBodySize + 6, 8); 
          }

          BitBufferHelper.writeBits(5, 8);  
          BitBufferHelper.writeBits(PDU_IEI_CONCATENATED_SHORT_MESSAGES_8BIT, 8);  
          BitBufferHelper.writeBits(3, 8);  
          BitBufferHelper.writeBits(options.segmentRef & 0xFF, 8);      
          BitBufferHelper.writeBits(options.segmentMaxSeq & 0xFF, 8);   
          BitBufferHelper.writeBits(options.segmentSeq & 0xFF, 8);      
        }
    } else {
      BitBufferHelper.writeBits(msgBodySize, 8);
    }

    
    const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
    const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
    for (let i = 0; i < msgBody.length; i++) {
      switch (options.encoding) {
        case PDU_CDMA_MSG_CODING_OCTET: {
          let msgDigit = msgBody.charCodeAt(i);
          BitBufferHelper.writeBits(msgDigit, 8);
          break;
        }
        case PDU_CDMA_MSG_CODING_7BITS_ASCII: {
          let msgDigit = msgBody.charCodeAt(i),
              msgDigitChar = msgBody.charAt(i);

          if (msgDigit >= 32) {
            BitBufferHelper.writeBits(msgDigit, 7);
          } else {
            msgDigit = langTable.indexOf(msgDigitChar);

            if (msgDigit === PDU_NL_EXTENDED_ESCAPE) {
              break;
            }
            if (msgDigit >= 0) {
              BitBufferHelper.writeBits(msgDigit, 7);
            } else {
              msgDigit = langShiftTable.indexOf(msgDigitChar);
              if (msgDigit == -1) {
                throw new Error("'" + msgDigitChar + "' is not in 7 bit alphabet "
                                + langIndex + ":" + langShiftIndex + "!");
                break;
              }

              if (msgDigit === PDU_NL_RESERVED_CONTROL) {
                break;
              }

              BitBufferHelper.writeBits(PDU_NL_EXTENDED_ESCAPE, 7);
              BitBufferHelper.writeBits(msgDigit, 7);
            }
          }
          break;
        }
        case PDU_CDMA_MSG_CODING_UNICODE: {
          let msgDigit = msgBody.charCodeAt(i);
          BitBufferHelper.writeBits(msgDigit, 16);
          break;
        }
      }
    }
    BitBufferHelper.flushWithPadding();

    
    let currentPosition = BitBufferHelper.getWriteBufferSize();
    BitBufferHelper.overwriteWriteBuffer(lengthPosition - 1, [currentPosition - lengthPosition]);
  },

  



  readMessage: function cdma_readMessage() {
    let message = {};

    
    message.teleservice = this.readInt();

    
    let isServicePresent = this.readByte();
    if (isServicePresent) {
      message.messageType = PDU_CDMA_MSG_TYPE_BROADCAST;
    } else {
      if (message.teleservice) {
        message.messageType = PDU_CDMA_MSG_TYPE_P2P;
      } else {
        message.messageType = PDU_CDMA_MSG_TYPE_ACK;
      }
    }

    
    message.service = this.readInt();

    
    let addrInfo = {};
    addrInfo.digitMode = (this.readInt() & 0x01);
    addrInfo.numberMode = (this.readInt() & 0x01);
    addrInfo.numberType = (this.readInt() & 0x01);
    addrInfo.numberPlan = (this.readInt() & 0x01);
    addrInfo.addrLength = this.readByte();
    addrInfo.address = [];
    for (let i = 0; i < addrInfo.addrLength; i++) {
      addrInfo.address.push(this.readByte());
    }
    message.sender = this.decodeAddr(addrInfo);

    
    addrInfo.Type = (this.readInt() & 0x07);
    addrInfo.Odd = (this.readByte() & 0x01);
    addrInfo.addrLength = this.readByte();
    for (let i = 0; i < addrInfo.addrLength; i++) {
      let addrDigit = this.readByte();
      message.sender += String.fromCharCode(addrDigit);
    }

    
    this.decodeUserData(message);

    
    let msg = {
      SMSC:           "",
      mti:            0,
      udhi:           0,
      sender:         message.sender,
      recipient:      null,
      pid:            PDU_PID_DEFAULT,
      epid:           PDU_PID_DEFAULT,
      dcs:            0,
      mwi:            null, 
      replace:        false,
      header:         message[PDU_CDMA_MSG_USERDATA_BODY].header,
      body:           message[PDU_CDMA_MSG_USERDATA_BODY].body,
      data:           null,
      timestamp:      message[PDU_CDMA_MSG_USERDATA_TIMESTAMP],
      status:         null,
      scts:           null,
      dt:             null,
      encoding:       message[PDU_CDMA_MSG_USERDATA_BODY].encoding,
      messageClass:   GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL]
    };

    return msg;
  },


  







  processReceivedSms: function cdma_processReceivedSms(length) {
    if (!length) {
      if (DEBUG) debug("Received empty SMS!");
      return [null, PDU_FCS_UNSPECIFIED];
    }

    let message = this.readMessage();
    if (DEBUG) debug("Got new SMS: " + JSON.stringify(message));

    
    if (!message) {
      return [null, PDU_FCS_UNSPECIFIED];
    }

    return [message, PDU_FCS_OK];
  },

  


  readInt: function readInt() {
    return Buf.readUint32();
  },

  readByte: function readByte() {
    return (Buf.readUint32() & 0xFF);
  },

  












  decodeAddr: function cdma_decodeAddr(addrInfo) {
    let result = "";
    for (let i = 0; i < addrInfo.addrLength; i++) {
      if (addrInfo.digitMode === PDU_CDMA_MSG_ADDR_DIGIT_MODE_DTMF) {
        result += this.dtmfChars.charAt(addrInfo.address[i]);
      } else {
        result += String.fromCharCode(addrInfo.address[i]);
      }
    }
    return result;
  },

  






  decodeUserData: function cdma_decodeUserData(message) {
    let userDataLength = this.readInt();

    while (userDataLength > 0) {
      let id = this.readByte(),
          length = this.readByte(),
          userDataBuffer = [];

      for (let i = 0; i < length; i++) {
          userDataBuffer.push(this.readByte());
      }

      BitBufferHelper.startRead(userDataBuffer);

      switch (id) {
        case PDU_CDMA_MSG_USERDATA_MSG_ID:
          message[id] = this.decodeUserDataMsgId();
          break;
        case PDU_CDMA_MSG_USERDATA_BODY:
          message[id] = this.decodeUserDataMsg(message[PDU_CDMA_MSG_USERDATA_MSG_ID].userHeader);
          break;
        case PDU_CDMA_MSG_USERDATA_TIMESTAMP:
          message[id] = this.decodeUserDataTimestamp();
          break;
        case PDU_CDMA_REPLY_OPTION:
          message[id] = this.decodeUserDataReplyAction();
          break;
        case PDU_CDMA_MSG_USERDATA_CALLBACK_NUMBER:
          message[id] = this.decodeUserDataCallbackNumber();
          break;
      }

      userDataLength -= (length + 2);
      userDataBuffer = [];
    }
  },

  




  decodeUserDataMsgId: function cdma_decodeUserDataMsgId() {
    let result = {};
    result.msgType = BitBufferHelper.readBits(4);
    result.msgId = BitBufferHelper.readBits(16);
    result.userHeader = BitBufferHelper.readBits(1);

    return result;
  },

  







  decodeUserDataHeader: function cdma_decodeUserDataHeader(encoding) {
    let header = {},
        headerSize = BitBufferHelper.readBits(8),
        userDataHeaderSize = headerSize + 1,
        headerPaddingBits = 0;

    
    if (encoding === PDU_DCS_MSG_CODING_7BITS_ALPHABET) {
      
      header.length = Math.ceil(userDataHeaderSize * 8 / 7);
      
      headerPaddingBits = (header.length * 7) - (userDataHeaderSize * 8);
    } else if (encoding === PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      header.length = userDataHeaderSize;
    } else {
      header.length = userDataHeaderSize / 2;
    }

    while (headerSize) {
      let identifier = BitBufferHelper.readBits(8),
          length = BitBufferHelper.readBits(8);

      headerSize -= (2 + length);

      switch (identifier) {
        case PDU_IEI_CONCATENATED_SHORT_MESSAGES_8BIT: {
            let ref = BitBufferHelper.readBits(8),
                max = BitBufferHelper.readBits(8),
                seq = BitBufferHelper.readBits(8);
            if (max && seq && (seq <= max)) {
              header.segmentRef = ref;
              header.segmentMaxSeq = max;
              header.segmentSeq = seq;
            }
          break;
        }
        case PDU_IEI_APPLICATION_PORT_ADDRESSING_SCHEME_8BIT: {
          let dstp = BitBufferHelper.readBits(8),
              orip = BitBufferHelper.readBits(8);
          if ((dstp < PDU_APA_RESERVED_8BIT_PORTS)
              || (orip < PDU_APA_RESERVED_8BIT_PORTS)) {
            
            
            
            break;
          }
          header.destinationPort = dstp;
          header.originatorPort = orip;
          break;
        }
        case PDU_IEI_APPLICATION_PORT_ADDRESSING_SCHEME_16BIT: {
          let dstp = (BitBufferHelper.readBits(8) << 8) | BitBufferHelper.readBits(8),
              orip = (BitBufferHelper.readBits(8) << 8) | BitBufferHelper.readBits(8);
          
          
          
          if ((dstp < PDU_APA_VALID_16BIT_PORTS)
              && (orip < PDU_APA_VALID_16BIT_PORTS)) {
            header.destinationPort = dstp;
            header.originatorPort = orip;
          }
          break;
        }
        case PDU_IEI_CONCATENATED_SHORT_MESSAGES_16BIT: {
          let ref = (BitBufferHelper.readBits(8) << 8) | BitBufferHelper.readBits(8),
              max = BitBufferHelper.readBits(8),
              seq = BitBufferHelper.readBits(8);
          if (max && seq && (seq <= max)) {
            header.segmentRef = ref;
            header.segmentMaxSeq = max;
            header.segmentSeq = seq;
          }
          break;
        }
        case PDU_IEI_NATIONAL_LANGUAGE_SINGLE_SHIFT: {
          let langShiftIndex = BitBufferHelper.readBits(8);
          if (langShiftIndex < PDU_NL_SINGLE_SHIFT_TABLES.length) {
            header.langShiftIndex = langShiftIndex;
          }
          break;
        }
        case PDU_IEI_NATIONAL_LANGUAGE_LOCKING_SHIFT: {
          let langIndex = BitBufferHelper.readBits(8);
          if (langIndex < PDU_NL_LOCKING_SHIFT_TABLES.length) {
            header.langIndex = langIndex;
          }
          break;
        }
        case PDU_IEI_SPECIAL_SMS_MESSAGE_INDICATION: {
          let msgInd = BitBufferHelper.readBits(8) & 0xFF,
              msgCount = BitBufferHelper.readBits(8);
          






          let storeType = msgInd & PDU_MWI_STORE_TYPE_BIT;
          header.mwi = {};
          mwi = header.mwi;

          if (storeType == PDU_MWI_STORE_TYPE_STORE) {
            
            
            mwi.discard = false;
          } else if (mwi.discard === undefined) {
            
            
            mwi.discard = true;
          }

          mwi.msgCount = msgCount & 0xFF;
          mwi.active = mwi.msgCount > 0;

          if (DEBUG) debug("MWI in TP_UDH received: " + JSON.stringify(mwi));

          break;
        }
        default: {
          
          for (let i = 0; i < length; i++) {
            BitBufferHelper.readBits(8);
          }
          break;
        }
      }
    }

    
    if (headerPaddingBits) {
      BitBufferHelper.readBits(headerPaddingBits);
    }

    return header;
  },

  




  decodeUserDataMsg: function cdma_decodeUserDataMsg(hasUserHeader) {
    let result = {},
        encoding = BitBufferHelper.readBits(5);

    if(encoding === PDU_CDMA_MSG_CODING_IS_91) {
      let msgType = BitBufferHelper.readBits(8);
    }

    let msgBodySize = BitBufferHelper.readBits(8);
    const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
    const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
    
    switch (encoding) {
      case PDU_CDMA_MSG_CODING_7BITS_ASCII:
      case PDU_CDMA_MSG_CODING_IA5:
      case PDU_CDMA_MSG_CODING_7BITS_GSM:
        result.encoding = PDU_DCS_MSG_CODING_7BITS_ALPHABET;
        break;
      case PDU_CDMA_MSG_CODING_OCTET:
      case PDU_CDMA_MSG_CODING_IS_91:
      case PDU_CDMA_MSG_CODING_LATIN_HEBREW:
      case PDU_CDMA_MSG_CODING_LATIN:
        result.encoding = PDU_DCS_MSG_CODING_8BITS_ALPHABET;
        break;
      case PDU_CDMA_MSG_CODING_UNICODE:
      case PDU_CDMA_MSG_CODING_SHIFT_JIS:
      case PDU_CDMA_MSG_CODING_KOREAN:
        result.encoding = PDU_DCS_MSG_CODING_16BITS_ALPHABET;
        break;
    }

    
    if (hasUserHeader) {
      result.header = this.decodeUserDataHeader(result.encoding);
      
      msgBodySize -= result.header.length;
    }

    
    result.body = "";
    let msgDigit;
    switch (encoding) {
      case PDU_CDMA_MSG_CODING_OCTET:         
        while(msgBodySize > 0) {
          msgDigit = String.fromCharCode(BitBufferHelper.readBits(8));
          result.body += msgDigit;
          msgBodySize--;
        }
        break;
      case PDU_CDMA_MSG_CODING_IS_91:         
        
        switch (msgType) {
          case PDU_CDMA_MSG_CODING_IS_91_TYPE_SMS:
          case PDU_CDMA_MSG_CODING_IS_91_TYPE_SMS_FULL:
          case PDU_CDMA_MSG_CODING_IS_91_TYPE_VOICEMAIL_STATUS:
            while(msgBodySize > 0) {
              msgDigit = String.fromCharCode(BitBufferHelper.readBits(6) + 0x20);
              result.body += msgDigit;
              msgBodySize--;
            }
            break;
          case PDU_CDMA_MSG_CODING_IS_91_TYPE_CLI:
            let addrInfo = {};
            addrInfo.digitMode = PDU_CDMA_MSG_ADDR_DIGIT_MODE_DTMF;
            addrInfo.numberMode = PDU_CDMA_MSG_ADDR_NUMBER_MODE_ANSI;
            addrInfo.numberType = PDU_CDMA_MSG_ADDR_NUMBER_TYPE_UNKNOWN;
            addrInfo.numberPlan = PDU_CDMA_MSG_ADDR_NUMBER_PLAN_UNKNOWN;
            addrInfo.addrLength = msgBodySize;
            addrInfo.address = [];
            for (let i = 0; i < addrInfo.addrLength; i++) {
              addrInfo.address.push(BitBufferHelper.readBits(4));
            }
            result.body = this.decodeAddr(addrInfo);
            break;
        }
      case PDU_CDMA_MSG_CODING_7BITS_ASCII:
      case PDU_CDMA_MSG_CODING_IA5:           
        while(msgBodySize > 0) {
          msgDigit = BitBufferHelper.readBits(7);
          if (msgDigit >= 32) {
            msgDigit = String.fromCharCode(msgDigit);
          } else {
            if (msgDigit !== PDU_NL_EXTENDED_ESCAPE) {
              msgDigit = langTable[msgDigit];
            } else {
              msgDigit = BitBufferHelper.readBits(7);
              msgBodySize--;
              msgDigit = langShiftTable[msgDigit];
            }
          }
          result.body += msgDigit;
          msgBodySize--;
        }
        break;
      case PDU_CDMA_MSG_CODING_UNICODE:
        while(msgBodySize > 0) {
          msgDigit = String.fromCharCode(BitBufferHelper.readBits(16));
          result.body += msgDigit;
          msgBodySize--;
        }
        break;
      case PDU_CDMA_MSG_CODING_7BITS_GSM:     
        while(msgBodySize > 0) {
          msgDigit = BitBufferHelper.readBits(7);
          if (msgDigit !== PDU_NL_EXTENDED_ESCAPE) {
            msgDigit = langTable[msgDigit];
          } else {
            msgDigit = BitBufferHelper.readBits(7);
            msgBodySize--;
            msgDigit = langShiftTable[msgDigit];
          }
          result.body += msgDigit;
          msgBodySize--;
        }
        break;
      case PDU_CDMA_MSG_CODING_LATIN:         
        
        while(msgBodySize > 0) {
          msgDigit = String.fromCharCode(BitBufferHelper.readBits(8));
          result.body += msgDigit;
          msgBodySize--;
        }
        break;
      case PDU_CDMA_MSG_CODING_LATIN_HEBREW:  
        
        while(msgBodySize > 0) {
          msgDigit = BitBufferHelper.readBits(8);
          if (msgDigit === 0xDF) {
            msgDigit = String.fromCharCode(0x2017);
          } else if (msgDigit === 0xFD) {
            msgDigit = String.fromCharCode(0x200E);
          } else if (msgDigit === 0xFE) {
            msgDigit = String.fromCharCode(0x200F);
          } else if (msgDigit >= 0xE0 && msgDigit <= 0xFA) {
            msgDigit = String.fromCharCode(0x4F0 + msgDigit);
          } else {
            msgDigit = String.fromCharCode(msgDigit);
          }
          result.body += msgDigit;
          msgBodySize--;
        }
        break;
      case PDU_CDMA_MSG_CODING_SHIFT_JIS:
        
        
      case PDU_CDMA_MSG_CODING_KOREAN:
      case PDU_CDMA_MSG_CODING_GSM_DCS:
      default:
        break;
    }

    return result;
  },

  decodeBcd: function cdma_decodeBcd(value) {
    return ((value >> 4) & 0xF) * 10 + (value & 0x0F);
  },

  




  decodeUserDataTimestamp: function cdma_decodeUserDataTimestamp() {
    let year = this.decodeBcd(BitBufferHelper.readBits(8)),
        month = this.decodeBcd(BitBufferHelper.readBits(8)) - 1,
        date = this.decodeBcd(BitBufferHelper.readBits(8)),
        hour = this.decodeBcd(BitBufferHelper.readBits(8)),
        min = this.decodeBcd(BitBufferHelper.readBits(8)),
        sec = this.decodeBcd(BitBufferHelper.readBits(8));

    if (year >= 96 && year <= 99) {
      year += 1900;
    } else {
      year += 2000;
    }

    let result = (new Date(year, month, date, hour, min, sec, 0)).valueOf();

    return result;
  },

  




  decodeUserDataReplyAction: function cdma_decodeUserDataReplyAction() {
    let replyAction = BitBufferHelper.readBits(4),
        result = { userAck: (replyAction & 0x8) ? true : false,
                   deliverAck: (replyAction & 0x4) ? true : false,
                   readAck: (replyAction & 0x2) ? true : false,
                   report: (replyAction & 0x1) ? true : false
                 };

    return result;
  },

  




  decodeUserDataCallbackNumber: function cdma_decodeUserDataCallbackNumber() {
    let digitMode = BitBufferHelper.readBits(1);
    if (digitMode) {
      let numberType = BitBufferHelper.readBits(3),
          numberPlan = BitBufferHelper.readBits(4);
    }
    let numberFields = BitBufferHelper.readBits(8),
        result = "";
    for (let i = 0; i < numberFields; i++) {
      if (digitMode === PDU_CDMA_MSG_ADDR_DIGIT_MODE_DTMF) {
        let addrDigit = BitBufferHelper.readBits(4);
        result += this.dtmfChars.charAt(addrDigit);
      } else {
        let addrDigit = BitBufferHelper.readBits(8);
        result += String.fromCharCode(addrDigit);
      }
    }

    return result;
  }
};

let StkCommandParamsFactory = {
  createParam: function createParam(cmdDetails, ctlvs) {
    let param;
    switch (cmdDetails.typeOfCommand) {
      case STK_CMD_REFRESH:
        param = this.processRefresh(cmdDetails, ctlvs);
        break;
      case STK_CMD_POLL_INTERVAL:
        param = this.processPollInterval(cmdDetails, ctlvs);
        break;
      case STK_CMD_POLL_OFF:
        param = this.processPollOff(cmdDetails, ctlvs);
        break;
      case STK_CMD_PROVIDE_LOCAL_INFO:
        param = this.processProvideLocalInfo(cmdDetails, ctlvs);
        break;
      case STK_CMD_SET_UP_EVENT_LIST:
        param = this.processSetUpEventList(cmdDetails, ctlvs);
        break;
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
      case STK_CMD_LAUNCH_BROWSER:
        param = this.processLaunchBrowser(cmdDetails, ctlvs);
        break;
      case STK_CMD_PLAY_TONE:
        param = this.processPlayTone(cmdDetails, ctlvs);
        break;
      case STK_CMD_TIMER_MANAGEMENT:
        param = this.processTimerManagement(cmdDetails, ctlvs);
        break;
      default:
        debug("unknown proactive command");
        break;
    }
    return param;
  },

  







  processRefresh: function processRefresh(cmdDetails, ctlvs) {
    let refreshType = cmdDetails.commandQualifier;
    switch (refreshType) {
      case STK_REFRESH_FILE_CHANGE:
      case STK_REFRESH_NAA_INIT_AND_FILE_CHANGE:
        let ctlv = StkProactiveCmdHelper.searchForTag(
          COMPREHENSIONTLV_TAG_FILE_LIST, ctlvs);
        if (ctlv) {
          let list = ctlv.value.fileList;
          if (DEBUG) {
            debug("Refresh, list = " + list);
          }
          ICCRecordHelper.fetchICCRecords();
        }
        break;
    }
    return null;
  },

  







  processPollInterval: function processPollInterval(cmdDetails, ctlvs) {
    let ctlv = StkProactiveCmdHelper.searchForTag(
        COMPREHENSIONTLV_TAG_DURATION, ctlvs);
    if (!ctlv) {
      RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Poll Interval: Required value missing : Duration");
    }

    return ctlv.value;
  },

  







  processPollOff: function processPollOff(cmdDetails, ctlvs) {
    return null;
  },

  







  processSetUpEventList: function processSetUpEventList(cmdDetails, ctlvs) {
    let ctlv = StkProactiveCmdHelper.searchForTag(
        COMPREHENSIONTLV_TAG_EVENT_LIST, ctlvs);
    if (!ctlv) {
      RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Event List: Required value missing : Event List");
    }

    return ctlv.value || {eventList: null};
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

    ctlv = StkProactiveCmdHelper.searchForTag(COMPREHENSIONTLV_TAG_DURATION, ctlvs);
    if (ctlv) {
      textMsg.duration = ctlv.value;
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

    
    ctlv = StkProactiveCmdHelper.searchForTag(
        COMPREHENSIONTLV_TAG_DURATION, ctlvs);
    if (ctlv) {
      input.duration = ctlv.value;
    }

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
          call.callMessage = ctlv.value.identifier;
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

    
    ctlv = StkProactiveCmdHelper.searchForTag(COMPREHENSIONTLV_TAG_DURATION, ctlvs);
    if (ctlv) {
      call.duration = ctlv.value;
    }

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
  },

  processPlayTone: function processPlayTone(cmdDetails, ctlvs) {
    let playTone = {};

    let ctlv = StkProactiveCmdHelper.searchForTag(
        COMPREHENSIONTLV_TAG_ALPHA_ID, ctlvs);
    if (ctlv) {
      playTone.text = ctlv.value.identifier;
    }

    ctlv = StkProactiveCmdHelper.searchForTag(COMPREHENSIONTLV_TAG_TONE, ctlvs);
    if (ctlv) {
      playTone.tone = ctlv.value.tone;
    }

    ctlv = StkProactiveCmdHelper.searchForTag(
        COMPREHENSIONTLV_TAG_DURATION, ctlvs);
    if (ctlv) {
      playTone.duration = ctlv.value;
    }

    
    playTone.isVibrate = (cmdDetails.commandQualifier & 0x01) != 0x00;

    return playTone;
  },

  







  processProvideLocalInfo: function processProvideLocalInfo(cmdDetails, ctlvs) {
    let provideLocalInfo = {
      localInfoType: cmdDetails.commandQualifier
    };
    return provideLocalInfo;
  },

  processTimerManagement: function processTimerManagement(cmdDetails, ctlvs) {
    let timer = {
      timerAction: cmdDetails.commandQualifier
    };

    let ctlv = StkProactiveCmdHelper.searchForTag(
        COMPREHENSIONTLV_TAG_TIMER_IDENTIFIER, ctlvs);
    if (ctlv) {
      timer.timerId = ctlv.value.timerId;
    }

    ctlv = StkProactiveCmdHelper.searchForTag(
        COMPREHENSIONTLV_TAG_TIMER_VALUE, ctlvs);
    if (ctlv) {
      timer.timerValue = ctlv.value.timerValue;
    }

    return timer;
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
      case COMPREHENSIONTLV_TAG_DURATION:
        return this.retrieveDuration(length);
      case COMPREHENSIONTLV_TAG_ADDRESS:
        return this.retrieveAddress(length);
      case COMPREHENSIONTLV_TAG_TEXT_STRING:
        return this.retrieveTextString(length);
      case COMPREHENSIONTLV_TAG_TONE:
        return this.retrieveTone(length);
      case COMPREHENSIONTLV_TAG_ITEM:
        return this.retrieveItem(length);
      case COMPREHENSIONTLV_TAG_ITEM_ID:
        return this.retrieveItemId(length);
      case COMPREHENSIONTLV_TAG_RESPONSE_LENGTH:
        return this.retrieveResponseLength(length);
      case COMPREHENSIONTLV_TAG_FILE_LIST:
        return this.retrieveFileList(length);
      case COMPREHENSIONTLV_TAG_DEFAULT_TEXT:
        return this.retrieveDefaultText(length);
      case COMPREHENSIONTLV_TAG_EVENT_LIST:
        return this.retrieveEventList(length);
      case COMPREHENSIONTLV_TAG_TIMER_IDENTIFIER:
        return this.retrieveTimerId(length);
      case COMPREHENSIONTLV_TAG_TIMER_VALUE:
        return this.retrieveTimerValue(length);
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

  








  retrieveDuration: function retrieveDuration(length) {
    let duration = {
      timeUnit: GsmPDUHelper.readHexOctet(),
      timeInterval: GsmPDUHelper.readHexOctet(),
    };
    return duration;
  },

  









  retrieveAddress: function retrieveAddress(length) {
    let address = {
      number : GsmPDUHelper.readDiallingNumber(length)
    };
    return address;
  },

  









  retrieveTextString: function retrieveTextString(length) {
    if (!length) {
      
      return {textString: null};
    }

    let text = {
      codingScheme: GsmPDUHelper.readHexOctet()
    };

    length--; 
    switch (text.codingScheme & 0x0f) {
      case STK_TEXT_CODING_GSM_7BIT_PACKED:
        text.textString = GsmPDUHelper.readSeptetsToString(length * 8 / 7, 0, 0, 0);
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

  







  retrieveTone: function retrieveTone(length) {
    let tone = {
      tone: GsmPDUHelper.readHexOctet(),
    };
    return tone;
  },

  









  retrieveItem: function retrieveItem(length) {
    
    
    
    
    if (!length) {
      return null;
    }
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

  









  retrieveFileList: function retrieveFileList(length) {
    let num = GsmPDUHelper.readHexOctet();
    let fileList = "";
    length--; 
    for (let i = 0; i < 2 * length; i++) {
      
      
      fileList += String.fromCharCode(Buf.readUint16());
    }
    return {
      fileList: fileList
    };
  },

  




  retrieveDefaultText: function retrieveDefaultText(length) {
    return this.retrieveTextString(length);
  },

  


  retrieveEventList: function retrieveEventList(length) {
    if (!length) {
      
      
      return null;
    }

    let eventList = [];
    for (let i = 0; i < length; i++) {
      eventList.push(GsmPDUHelper.readHexOctet());
    }
    return {
      eventList: eventList
    };
  },

  







  retrieveTimerId: function retrieveTimerId(length) {
    let id = {
      timerId: GsmPDUHelper.readHexOctet()
    };
    return id;
  },

  









  retrieveTimerValue: function retrieveTimerValue(length) {
    let value = {
      timerValue: (GsmPDUHelper.readSwappedNibbleBcdNum(1) * 60 * 60) +
                  (GsmPDUHelper.readSwappedNibbleBcdNum(1) * 60) +
                  (GsmPDUHelper.readSwappedNibbleBcdNum(1))
    };
    return value;
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
  },

  




  writeLocationInfoTlv: function writeLocationInfoTlv(loc) {
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_LOCATION_INFO |
                               COMPREHENSIONTLV_FLAG_CR);
    GsmPDUHelper.writeHexOctet(loc.gsmCellId > 0xffff ? 9 : 7);
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    let mcc = loc.mcc.toString();
    let mnc = loc.mnc.toString();
    if (mnc.length == 1) {
      mnc = "F0" + mnc;
    } else if (mnc.length == 2) {
      mnc = "F" + mnc;
    } else {
      mnc = mnc[2] + mnc[0] + mnc[1];
    }
    GsmPDUHelper.writeSwappedNibbleBCD(mcc + mnc);

    
    GsmPDUHelper.writeHexOctet((loc.gsmLocationAreaCode >> 8) & 0xff);
    GsmPDUHelper.writeHexOctet(loc.gsmLocationAreaCode & 0xff);

    
    if (loc.gsmCellId > 0xffff) {
      
      GsmPDUHelper.writeHexOctet((loc.gsmCellId >> 24) & 0xff);
      GsmPDUHelper.writeHexOctet((loc.gsmCellId >> 16) & 0xff);
      GsmPDUHelper.writeHexOctet((loc.gsmCellId >> 8) & 0xff);
      GsmPDUHelper.writeHexOctet(loc.gsmCellId & 0xff);
    } else {
      
      GsmPDUHelper.writeHexOctet((loc.gsmCellId >> 8) & 0xff);
      GsmPDUHelper.writeHexOctet(loc.gsmCellId & 0xff);
    }
  },

  





  writeCauseTlv: function writeCauseTlv(geckoError) {
    let cause = -1;
    for (let errorNo in RIL_ERROR_TO_GECKO_ERROR) {
      if (geckoError == RIL_ERROR_TO_GECKO_ERROR[errorNo]) {
        cause = errorNo;
        break;
      }
    }
    cause = (cause == -1) ? ERROR_SUCCESS : cause;

    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_CAUSE |
                               COMPREHENSIONTLV_FLAG_CR);
    GsmPDUHelper.writeHexOctet(2);  

    
    GsmPDUHelper.writeHexOctet(0x60);

    
    
    
    
    GsmPDUHelper.writeHexOctet(0x80 | cause);
  },

  writeDateTimeZoneTlv: function writeDataTimeZoneTlv(date) {
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_DATE_TIME_ZONE);
    GsmPDUHelper.writeHexOctet(7);
    GsmPDUHelper.writeTimestamp(date);
  },

  writeLanguageTlv: function writeLanguageTlv(language) {
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_LANGUAGE);
    GsmPDUHelper.writeHexOctet(2);

    
    
    GsmPDUHelper.writeHexOctet(
      PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT].indexOf(language[0]));
    GsmPDUHelper.writeHexOctet(
      PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT].indexOf(language[1]));
  },

  





  writeTimerValueTlv: function writeTimerValueTlv(seconds, cr) {
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_TIMER_VALUE |
                               (cr ? COMPREHENSIONTLV_FLAG_CR : 0));
    GsmPDUHelper.writeHexOctet(3);

    
    
    
    
    GsmPDUHelper.writeSwappedNibbleBCDNum(Math.floor(seconds / 60 / 60));
    GsmPDUHelper.writeSwappedNibbleBCDNum(Math.floor(seconds / 60) % 60);
    GsmPDUHelper.writeSwappedNibbleBCDNum(seconds % 60);
  },

  getSizeOfLengthOctets: function getSizeOfLengthOctets(length) {
    if (length >= 0x10000) {
      return 4; 
    } else if (length >= 0x100) {
      return 3; 
    } else if (length >= 0x80) {
      return 2; 
    } else {
      return 1; 
    }
  },

  writeLength: function writeLength(length) {
    
    
    
    
    
    
    
    if (length < 0x80) {
      GsmPDUHelper.writeHexOctet(length);
    } else if (0x80 <= length && length < 0x100) {
      GsmPDUHelper.writeHexOctet(0x81);
      GsmPDUHelper.writeHexOctet(length);
    } else if (0x100 <= length && length < 0x10000) {
      GsmPDUHelper.writeHexOctet(0x82);
      GsmPDUHelper.writeHexOctet((length >> 8) & 0xff);
      GsmPDUHelper.writeHexOctet(length & 0xff);
    } else if (0x10000 <= length && length < 0x1000000) {
      GsmPDUHelper.writeHexOctet(0x83);
      GsmPDUHelper.writeHexOctet((length >> 16) & 0xff);
      GsmPDUHelper.writeHexOctet((length >> 8) & 0xff);
      GsmPDUHelper.writeHexOctet(length & 0xff);
    } else {
      throw new Error("Invalid length value :" + length);
    }
  },
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




let ICCFileHelper = {
  



  getCommonEFPath: function getCommonEFPath(fileId) {
    switch (fileId) {
      case ICC_EF_ICCID:
        return EF_PATH_MF_SIM;
      case ICC_EF_ADN:
        return EF_PATH_MF_SIM + EF_PATH_DF_TELECOM;
      case ICC_EF_PBR:
        return EF_PATH_MF_SIM + EF_PATH_DF_TELECOM + EF_PATH_DF_PHONEBOOK;
    }
    return null;
  },

  


  getSimEFPath: function getSimEFPath(fileId) {
    switch (fileId) {
      case ICC_EF_FDN:
      case ICC_EF_MSISDN:
        return EF_PATH_MF_SIM + EF_PATH_DF_TELECOM;
      case ICC_EF_AD:
      case ICC_EF_MBDN:
      case ICC_EF_PLMNsel:
      case ICC_EF_SPN:
      case ICC_EF_SPDI:
      case ICC_EF_SST:
      case ICC_EF_PHASE:
      case ICC_EF_CBMI:
      case ICC_EF_CBMIR:
      case ICC_EF_OPL:
      case ICC_EF_PNN:
        return EF_PATH_MF_SIM + EF_PATH_DF_GSM;
      default:
        return null
    }
  },

  


  getUSimEFPath: function getUSimEFPath(fileId) {
    switch (fileId) {
      case ICC_EF_AD:
      case ICC_EF_FDN:
      case ICC_EF_MBDN:
      case ICC_EF_UST:
      case ICC_EF_MSISDN:
      case ICC_EF_SPN:
      case ICC_EF_SPDI:
      case ICC_EF_CBMI:
      case ICC_EF_CBMIR:
      case ICC_EF_OPL:
      case ICC_EF_PNN:
        return EF_PATH_MF_SIM + EF_PATH_ADF_USIM;
      default:
        
        
        
        return EF_PATH_MF_SIM + EF_PATH_DF_TELECOM + EF_PATH_DF_PHONEBOOK;
    }
  },

  


  getRuimEFPath: function getRuimEFPath(fileId) {
    switch(fileId) {
      case ICC_EF_CSIM_CDMAHOME:
      case ICC_EF_CSIM_CST:
        return EF_PATH_MF_SIM + EF_PATH_DF_CDMA;
      default:
        return null;
    }
  },

  







  getEFPath: function getEFPath(fileId) {
    if (RIL.appType == null) {
      return null;
    }

    let path = this.getCommonEFPath(fileId);
    if (path) {
      return path;
    }

    switch (RIL.appType) {
      case CARD_APPTYPE_SIM:
        return this.getSimEFPath(fileId);
      case CARD_APPTYPE_USIM:
        return this.getUSimEFPath(fileId);
      case CARD_APPTYPE_RUIM:
        return this.getRuimEFPath(fileId);
      default:
        return null;
    }
  }
};




let ICCIOHelper = {
  













  loadLinearFixedEF: function loadLinearFixedEF(options) {
    options.type = EF_TYPE_LINEAR_FIXED;
    let cb = options.callback;
    options.callback = function callback(options) {
      options.callback = cb;
      options.command = ICC_COMMAND_READ_RECORD;
      options.p1 = options.recordNumber || 1; 
      options.p2 = READ_RECORD_ABSOLUTE_MODE;
      options.p3 = options.recordSize;
      RIL.iccIO(options);
    }.bind(this);
    this.getResponse(options);
  },

  


  loadNextRecord: function loadNextRecord(options) {
    options.p1++;
    RIL.iccIO(options);
  },

  













  updateLinearFixedEF: function updateLinearFixedEF(options) {
    if (!options.fileId || !options.recordNumber) {
      throw new Error("Unexpected fileId " + options.fileId +
                      " or recordNumber " + options.recordNumber);
    }

    options.type = EF_TYPE_LINEAR_FIXED;
    let cb = options.callback;
    options.callback = function callback(options) {
      options.callback = cb;
      options.command = ICC_COMMAND_UPDATE_RECORD;
      options.p1 = options.recordNumber;
      options.p2 = READ_RECORD_ABSOLUTE_MODE;
      options.p3 = options.recordSize;
      RIL.iccIO(options);
    }.bind(this);
    this.getResponse(options);
  },

  









  loadTransparentEF: function loadTransparentEF(options) {
    options.type = EF_TYPE_TRANSPARENT;
    let cb = options.callback;
    options.callback = function callback(options) {
      options.callback = cb;
      options.command = ICC_COMMAND_READ_BINARY;
      options.p3 = options.fileSize;
      RIL.iccIO(options);
    }.bind(this);
    this.getResponse(options);
  },

  





  getResponse: function getResponse(options) {
    options.command = ICC_COMMAND_GET_RESPONSE;
    options.pathId = ICCFileHelper.getEFPath(options.fileId);
    if (!options.pathId) {
      throw new Error("Unknown pathId for " + options.fileId.toString(16));
    }
    options.p1 = 0; 
    options.p2 = 0; 
    options.p3 = GET_RESPONSE_EF_SIZE_BYTES;
    RIL.iccIO(options);
  },

  


  processICCIO: function processICCIO(options) {
    let func = this[options.command];
    func.call(this, options);
  },

  


  processICCIOGetResponse: function processICCIOGetResponse(options) {
    let length = Buf.readUint32();

    

    
    Buf.seekIncoming(2 * PDU_HEX_OCTET_SIZE);

    
    options.fileSize = (GsmPDUHelper.readHexOctet() << 8) |
                        GsmPDUHelper.readHexOctet();

    
    let fileId = (GsmPDUHelper.readHexOctet() << 8) |
                  GsmPDUHelper.readHexOctet();
    if (fileId != options.fileId) {
      throw new Error("Expected file ID " + options.fileId.toString(16) +
                      " but read " + fileId.toString(16));
    }

    
    let fileType = GsmPDUHelper.readHexOctet();
    if (fileType != TYPE_EF) {
      throw new Error("Unexpected file type " + fileType);
    }

    
    
    
    
    Buf.seekIncoming(((RESPONSE_DATA_STRUCTURE - RESPONSE_DATA_FILE_TYPE - 1) *
        PDU_HEX_OCTET_SIZE));

    
    let efType = GsmPDUHelper.readHexOctet();
    if (efType != options.type) {
      throw new Error("Expected EF type " + options.type + " but read " + efType);
    }

    
    
    if (efType == EF_TYPE_LINEAR_FIXED || efType == EF_TYPE_CYCLIC) {
      options.recordSize = GsmPDUHelper.readHexOctet();
      options.totalRecords = options.fileSize / options.recordSize;
    } else {
      Buf.seekIncoming(1 * PDU_HEX_OCTET_SIZE);
    }

    Buf.readStringDelimiter(length);

    if (options.callback) {
      options.callback(options);
    }
  },

  


  processICCIOReadRecord: function processICCIOReadRecord(options) {
    if (options.callback) {
      options.callback(options);
    }
  },

  


  processICCIOReadBinary: function processICCIOReadBinary(options) {
    if (options.callback) {
      options.callback(options);
    }
  },

  


  processICCIOUpdateRecord: function processICCIOUpdateRecord(options) {
    if (options.callback) {
      options.callback(options);
    }
  },

  


  processICCIOError: function processICCIOError(options) {
    let error = options.onerror || debug;

    
    
    let errorMsg = "ICC I/O Error code " +
                   RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError] +
                   " EF id = " + options.fileId.toString(16) +
                   " command = " + options.command.toString(16);
    if (options.sw1 && options.sw2) {
      errorMsg += "(" + options.sw1.toString(16) +
                  "/" + options.sw2.toString(16) + ")";
    }
    error(errorMsg);
  },
};
ICCIOHelper[ICC_COMMAND_SEEK] = null;
ICCIOHelper[ICC_COMMAND_READ_BINARY] = function ICC_COMMAND_READ_BINARY(options) {
  this.processICCIOReadBinary(options);
};
ICCIOHelper[ICC_COMMAND_READ_RECORD] = function ICC_COMMAND_READ_RECORD(options) {
  this.processICCIOReadRecord(options);
};
ICCIOHelper[ICC_COMMAND_GET_RESPONSE] = function ICC_COMMAND_GET_RESPONSE(options) {
  this.processICCIOGetResponse(options);
};
ICCIOHelper[ICC_COMMAND_UPDATE_BINARY] = null;
ICCIOHelper[ICC_COMMAND_UPDATE_RECORD] = function ICC_COMMAND_UPDATE_RECORD(options) {
  this.processICCIOUpdateRecord(options);
};




let ICCRecordHelper = {
  


  fetchICCRecords: function fetchICCRecords() {
    this.getICCID();
    RIL.getIMSI();
    this.getMSISDN();
    this.getAD();
    this.getSST();
    this.getMBDN();
  },

  



  getICCPhase: function getICCPhase() {
    function callback() {
      let length = Buf.readUint32();

      let phase = GsmPDUHelper.readHexOctet();
      
      
      if (phase >= ICC_PHASE_2_PROFILE_DOWNLOAD_REQUIRED) {
        RIL.sendStkTerminalProfile(STK_SUPPORTED_TERMINAL_PROFILE);
      }

      Buf.readStringDelimiter(length);
    }

    ICCIOHelper.loadTransparentEF({fileId: ICC_EF_PHASE,
                                   callback: callback.bind(this)});
  },

  


  getICCID: function getICCID() {
    function callback() {
      let length = Buf.readUint32();
      RIL.iccInfo.iccid = GsmPDUHelper.readSwappedNibbleBcdString(length / 2);
      Buf.readStringDelimiter(length);

      if (DEBUG) debug("ICCID: " + RIL.iccInfo.iccid);
      if (RIL.iccInfo.iccid) {
        ICCUtilsHelper.handleICCInfoChange();
      }
    }

    ICCIOHelper.loadTransparentEF({fileId: ICC_EF_ICCID,
                                   callback: callback.bind(this)});
  },

  


  getMSISDN: function getMSISDN() {
    function callback(options) {
      let contact = GsmPDUHelper.readAlphaIdDiallingNumber(options.recordSize);
      if (!contact || RIL.iccInfo.msisdn === contact.number) {
        return;
      }
      RIL.iccInfo.msisdn = contact.number;
      if (DEBUG) debug("MSISDN: " + RIL.iccInfo.msisdn);
      ICCUtilsHelper.handleICCInfoChange();
    }

    ICCIOHelper.loadLinearFixedEF({fileId: ICC_EF_MSISDN,
                                   callback: callback.bind(this)});
  },

  


  getAD: function getAD() {
    function callback() {
      let length = Buf.readUint32();
      
      let len = length / 2;
      let ad = GsmPDUHelper.readHexOctetArray(len);
      Buf.readStringDelimiter(length);

      if (DEBUG) {
        let str = "";
        for (let i = 0; i < ad.length; i++) {
          str += ad[i] + ", ";
        }
        debug("AD: " + str);
      }

      let imsi = RIL.iccInfoPrivate.imsi;
      if (imsi) {
        
        RIL.iccInfo.mcc = parseInt(imsi.substr(0,3));
        
        RIL.iccInfo.mnc = parseInt(imsi.substr(3, ad[3]));
        if (DEBUG) debug("MCC: " + RIL.iccInfo.mcc + " MNC: " + RIL.iccInfo.mnc);
        ICCUtilsHelper.handleICCInfoChange();
      }
    }

    ICCIOHelper.loadTransparentEF({fileId: ICC_EF_AD,
                                   callback: callback.bind(this)});
  },

  


  getSPN: function getSPN() {
    function callback() {
      let length = Buf.readUint32();
      
      
      let len = (length / 2) - 1;
      let spnDisplayCondition = GsmPDUHelper.readHexOctet();
      let spn = GsmPDUHelper.readAlphaIdentifier(len);
      Buf.readStringDelimiter(length);

      if (DEBUG) {
        debug("SPN: spn = " + spn +
              ", spnDisplayCondition = " + spnDisplayCondition);
      }

      RIL.iccInfoPrivate.SPN = {
        spn : spn,
        spnDisplayCondition : spnDisplayCondition,
      };
      RIL.iccInfo.spn = spn;
      ICCUtilsHelper.updateDisplayCondition();
      ICCUtilsHelper.handleICCInfoChange();
    }

    ICCIOHelper.loadTransparentEF({fileId: ICC_EF_SPN,
                                   callback: callback.bind(this)});
  },

  


  getSST: function getSST() {
    function callback() {
      let length = Buf.readUint32();
      
      let len = length / 2;
      let sst = GsmPDUHelper.readHexOctetArray(len);
      Buf.readStringDelimiter(length);
      RIL.iccInfoPrivate.sst = sst;
      if (DEBUG) {
        let str = "";
        for (let i = 0; i < sst.length; i++) {
          str += sst[i] + ", ";
        }
        debug("SST: " + str);
      }

      
      if (ICCUtilsHelper.isICCServiceAvailable("SPN")) {
        if (DEBUG) debug("SPN: SPN is available");
        this.getSPN();
      } else {
        if (DEBUG) debug("SPN: SPN service is not available");
      }

      if (ICCUtilsHelper.isICCServiceAvailable("SPDI")) {
        if (DEBUG) debug("SPDI: SPDI available.");
        this.getSPDI();
      } else {
        if (DEBUG) debug("SPDI: SPDI service is not available");
      }

      if (ICCUtilsHelper.isICCServiceAvailable("PNN")) {
        if (DEBUG) debug("PNN: PNN is available");
        this.readPNN();
      } else {
        if (DEBUG) debug("PNN: PNN is not available");
      }

      if (ICCUtilsHelper.isICCServiceAvailable("OPL")) {
        if (DEBUG) debug("OPL: OPL is available");
        this.getOPL();
      } else {
        if (DEBUG) debug("OPL: OPL is not available");
      }

      if (ICCUtilsHelper.isICCServiceAvailable("CBMI")) {
        this.getCBMI();
      } else {
        RIL.cellBroadcastConfigs.CBMI = null;
      }
      if (ICCUtilsHelper.isICCServiceAvailable("CBMIR")) {
        this.getCBMIR();
      } else {
        RIL.cellBroadcastConfigs.CBMIR = null;
      }
      RIL._mergeAllCellBroadcastConfigs();
    }

    
    ICCIOHelper.loadTransparentEF({fileId: ICC_EF_SST,
                                   callback: callback.bind(this)});
  },

  





  readFDN: function readFDN(onsuccess, onerror) {
    function callback(options) {
      let contact = GsmPDUHelper.readAlphaIdDiallingNumber(options.recordSize);
      if (contact) {
        contacts.push(contact);
      }

      if (options.p1 < options.totalRecords) {
        ICCIOHelper.loadNextRecord(options);
      } else {
        if (onsuccess) {
          onsuccess(contacts);
        }
      }
    }

    let contacts = [];
    ICCIOHelper.loadLinearFixedEF({fileId: ICC_EF_FDN,
                                   callback: callback.bind(this),
                                   onerror: onerror});
  },

  






  readADN: function readADN(fileId, onsuccess, onerror) {
    function callback(options) {
      let contact = GsmPDUHelper.readAlphaIdDiallingNumber(options.recordSize);
      if (contact) {
        contact.recordId = options.p1;
        contacts.push(contact);
      }

      if (options.p1 < options.totalRecords) {
        ICCIOHelper.loadNextRecord(options);
      } else {
        if (DEBUG) {
          for (let i = 0; i < contacts.length; i++) {
            debug("ADN[" + i + "] " + JSON.stringify(contacts[i]));
          }
        }
        if (onsuccess) {
          onsuccess(contacts);
        }
      }
    }

    let contacts = [];
    ICCIOHelper.loadLinearFixedEF({fileId: fileId,
                                   callback: callback.bind(this),
                                   onerror: onerror});
  },

  







  updateADN: function updateADN(fileId, contact, onsuccess, onerror) {
    function dataWriter(recordSize) {
      GsmPDUHelper.writeAlphaIdDiallingNumber(recordSize,
                                              contact.alphaId,
                                              contact.number);
    }

    function callback(options) {
      if (onsuccess) {
        onsuccess();
      }
    }

    if (!contact || !contact.recordId) {
      if (onerror) {
        onerror("Invalid parameter.");
      }
      return;
    }

    ICCIOHelper.updateLinearFixedEF({fileId: fileId,
                                     recordNumber: contact.recordId,
                                     dataWriter: dataWriter.bind(this),
                                     callback: callback.bind(this),
                                     onerror: onerror});
  },

  




  getMBDN: function getMBDN() {
    function callback(options) {
      let contact = GsmPDUHelper.readAlphaIdDiallingNumber(options.recordSize);
      if (!contact || RIL.iccInfo.mbdn === contact.number){
        return;
      }
      RIL.iccInfo.mbdn = contact.number;
      if (DEBUG) {
        debug("MBDN, alphaId="+contact.alphaId+" number="+contact.number);
      }
      contact.rilMessageType = "iccmbdn";
      RIL.sendDOMMessage(contact);
    }

    ICCIOHelper.loadLinearFixedEF({fileId: ICC_EF_MBDN,
                                   callback: callback.bind(this)});
  },

  





  readPBR: function readPBR(onsuccess, onerror) {
    function callback(options) {
      let strLen = Buf.readUint32();
      let octetLen = strLen / 2, readLen = 0;

      let pbrTlvs = [];
      while (readLen < octetLen) {
        let tag = GsmPDUHelper.readHexOctet();
        if (tag == 0xff) {
          readLen++;
          Buf.seekIncoming((octetLen - readLen) * PDU_HEX_OCTET_SIZE);
          break;
        }

        let tlvLen = GsmPDUHelper.readHexOctet();
        let tlvs = ICCUtilsHelper.decodeSimTlvs(tlvLen);
        pbrTlvs.push({tag: tag,
                      length: tlvLen,
                      value: tlvs});

        readLen += tlvLen + 2; 
      }
      Buf.readStringDelimiter(strLen);

      if (pbrTlvs.length == 0) {
        let error = onerror || debug;
        error("Cannot access Phonebook.");
        return;
      }

      let pbr = ICCUtilsHelper.parsePbrTlvs(pbrTlvs);
      if (onsuccess) {
        onsuccess(pbr);
      }
    }

    ICCIOHelper.loadLinearFixedEF({fileId : ICC_EF_PBR,
                                   callback: callback.bind(this),
                                   onerror: onerror});
  },

  









  readIAP: function readIAP(fileId, recordNumber, onsuccess, onerror) {
    function callback(options) {
      let strLen = Buf.readUint32();
      let octetLen = strLen / 2;

      let iap = GsmPDUHelper.readHexOctetArray(octetLen);
      Buf.readStringDelimiter(strLen);

      if (onsuccess) {
        onsuccess(iap);
      }
    }

    ICCIOHelper.loadLinearFixedEF({fileId: fileId,
                                   recordNumber: recordNumber,
                                   callback: callback.bind(this),
                                   onerror: onerror});
  },

  










  readEmail: function readEmail(fileId, fileType, recordNumber, onsuccess, onerror) {
    function callback(optoins) {
      let strLen = Buf.readUint32();
      let octetLen = strLen / 2;
      let email = null;

      
      
      
      
      
      
      
      
      if (fileType == ICC_USIM_TYPE1_TAG) {
        email = GsmPDUHelper.read8BitUnpackedToString(octetLen);
      } else {
        email = GsmPDUHelper.read8BitUnpackedToString(octetLen - 2);

        
        Buf.seekIncoming(2 * PDU_HEX_OCTET_SIZE); 
      }

      Buf.readStringDelimiter(strLen);

      if (onsuccess) {
        onsuccess(email);
      }
    }

    ICCIOHelper.loadLinearFixedEF({fileId: fileId,
                                   recordNumber: recordNumber,
                                   callback: callback.bind(this),
                                   onerror: onerror});
  },

  




  getPLMNSelector: function getPLMNSelector() {
    function callback() {
      if (DEBUG) debug("PLMN Selector: Process PLMN Selector");

      let length = Buf.readUint32();
      RIL.iccInfoPrivate.PLMN = this.readPLMNEntries(length/3);
      Buf.readStringDelimiter(length);

      if (DEBUG) debug("PLMN Selector: " + JSON.stringify(RIL.iccInfoPrivate.PLMN));

      if (RIL.updateDisplayCondition()) {
        this.handleICCInfoChange();
      }
    }

    
    ICCIOHelper.loadTransparentEF({fileId: ICC_EF_PLMNsel,
                                   callback: callback.bind(this)});
  },

  





  getSPDI: function getSPDI() {
    function callback() {
      let length = Buf.readUint32();
      let readLen = 0;
      let endLoop = false;
      RIL.iccInfoPrivate.SPDI = null;
      while ((readLen < length / 2) && !endLoop) {
        let tlvTag = GsmPDUHelper.readHexOctet();
        let tlvLen = GsmPDUHelper.readHexOctet();
        readLen += 2; 
        switch (tlvTag) {
        case SPDI_TAG_SPDI:
          
          continue;
        case SPDI_TAG_PLMN_LIST:
          
          RIL.iccInfoPrivate.SPDI = this.readPLMNEntries(tlvLen / 3);
          readLen += tlvLen;
          endLoop = true;
          break;
        default:
          
          
          endLoop = true;
          break;
        }
      }

      
      Buf.seekIncoming((length / 2 - readLen) * PDU_HEX_OCTET_SIZE);
      Buf.readStringDelimiter(length);

      if (DEBUG) debug("SPDI: " + JSON.stringify(RIL.iccInfoPrivate.SPDI));
      if (ICCUtilsHelper.updateDisplayCondition()) {
        ICCUtilsHelper.handleICCInfoChange();
      }
    }

    
    ICCIOHelper.loadTransparentEF({fileId: ICC_EF_SPDI,
                                   callback: callback.bind(this)});
  },

  




  getCBMI: function getCBMI() {
    function callback() {
      let strLength = Buf.readUint32();

      
      
      let numIds = strLength / 4, list = null;
      if (numIds) {
        list = [];
        for (let i = 0, id; i < numIds; i++) {
          id = GsmPDUHelper.readHexOctet() << 8 | GsmPDUHelper.readHexOctet();
          
          if (id != 0xFFFF) {
            list.push(id);
            list.push(id + 1);
          }
        }
      }
      if (DEBUG) {
        debug("CBMI: " + JSON.stringify(list));
      }

      Buf.readStringDelimiter(strLength);

      RIL.cellBroadcastConfigs.CBMI = list;
      RIL._mergeAllCellBroadcastConfigs();
    }

    function onerror() {
      RIL.cellBroadcastConfigs.CBMI = null;
      RIL._mergeAllCellBroadcastConfigs();
    }

    ICCIOHelper.loadTransparentEF({fileId: ICC_EF_CBMI,
                                   callback: callback.bind(this),
                                   onerror: onerror.bind(this)});
  },

  




  getCBMIR: function getCBMIR() {
    function callback() {
      let strLength = Buf.readUint32();

      
      
      let numIds = strLength / 8, list = null;
      if (numIds) {
        list = [];
        for (let i = 0, from, to; i < numIds; i++) {
          
          
          
          from = GsmPDUHelper.readHexOctet() << 8 | GsmPDUHelper.readHexOctet();
          to = GsmPDUHelper.readHexOctet() << 8 | GsmPDUHelper.readHexOctet();
          
          if ((from != 0xFFFF) && (to != 0xFFFF)) {
            list.push(from);
            list.push(to + 1);
          }
        }
      }
      if (DEBUG) {
        debug("CBMIR: " + JSON.stringify(list));
      }

      Buf.readStringDelimiter(strLength);

      RIL.cellBroadcastConfigs.CBMIR = list;
      RIL._mergeAllCellBroadcastConfigs();
    }

    function onerror() {
      RIL.cellBroadcastConfigs.CBMIR = null;
      RIL._mergeAllCellBroadcastConfigs();
    }

    ICCIOHelper.loadTransparentEF({fileId: ICC_EF_CBMIR,
                                   callback: callback.bind(this),
                                   onerror: onerror.bind(this)});
  },

  





  getOPL: function getOPL() {
    let opl = [];
    function callback(options) {
      let len = Buf.readUint32();
      
      
      
      
      
      
      
      let mccMnc = [GsmPDUHelper.readHexOctet(),
                    GsmPDUHelper.readHexOctet(),
                    GsmPDUHelper.readHexOctet()];
      if (mccMnc[0] != 0xFF || mccMnc[1] != 0xFF || mccMnc[2] != 0xFF) {
        let oplElement = {};
        let semiOctets = [];
        for (let i = 0; i < mccMnc.length; i++) {
          semiOctets.push((mccMnc[i] & 0xf0) >> 4);
          semiOctets.push(mccMnc[i] & 0x0f);
        }
        let reformat = [semiOctets[1], semiOctets[0], semiOctets[3],
                        semiOctets[5], semiOctets[4], semiOctets[2]];
        let buf = "";
        for (let i = 0; i < reformat.length; i++) {
          if (reformat[i] != 0xF) {
            buf += GsmPDUHelper.semiOctetToBcdChar(reformat[i]);
          }
          if (i === 2) {
            
            oplElement.mcc = parseInt(buf);
            buf = "";
          } else if (i === 5) {
            
            oplElement.mnc = parseInt(buf);
          }
        }
        
        oplElement.lacTacStart =
          (GsmPDUHelper.readHexOctet() << 8) | GsmPDUHelper.readHexOctet();
        oplElement.lacTacEnd =
          (GsmPDUHelper.readHexOctet() << 8) | GsmPDUHelper.readHexOctet();
        
        oplElement.pnnRecordId = GsmPDUHelper.readHexOctet();
        if (DEBUG) {
          debug("OPL: [" + (opl.length + 1) + "]: " + JSON.stringify(oplElement));
        }
        opl.push(oplElement);
      } else {
        Buf.seekIncoming(5 * PDU_HEX_OCTET_SIZE);
      }
      Buf.readStringDelimiter(len);

      if (options.p1 < options.totalRecords) {
        ICCIOHelper.loadNextRecord(options);
      } else {
        RIL.iccInfoPrivate.OPL = opl;
      }
    }

    ICCIOHelper.loadLinearFixedEF({fileId: ICC_EF_OPL,
                                   callback: callback.bind(this)});
  },

  





  readPNN: function readPNN() {
    function callback(options) {
      let pnnElement;
      let strLen = Buf.readUint32();
      let octetLen = strLen / 2;
      let readLen = 0;

      while (readLen < octetLen) {
        let tlvTag = GsmPDUHelper.readHexOctet();

        if (tlvTag == 0xFF) {
          
          readLen++;
          Buf.seekIncoming((octetLen - readLen) * PDU_HEX_OCTET_SIZE);
          break;
        }

        
        pnnElement = pnnElement || {};

        let tlvLen = GsmPDUHelper.readHexOctet();

        switch (tlvTag) {
          case PNN_IEI_FULL_NETWORK_NAME:
            pnnElement.fullName = GsmPDUHelper.readNetworkName(tlvLen);
            break;
          case PNN_IEI_SHORT_NETWORK_NAME:
            pnnElement.shortName = GsmPDUHelper.readNetworkName(tlvLen);
            break;
          default:
            Buf.seekIncoming(tlvLen * PDU_HEX_OCTET_SIZE);
            break;
        }

        readLen += (tlvLen + 2); 
      }
      Buf.readStringDelimiter(strLen);

      if (pnnElement) {
        pnn.push(pnnElement);
      }

      
      if (pnnElement && options.p1 < options.totalRecords) {
        ICCIOHelper.loadNextRecord(options);
      } else {
        if (DEBUG) {
          for (let i = 0; i < pnn.length; i++) {
            debug("PNN: [" + i + "]: " + JSON.stringify(pnn[i]));
          }
        }
        RIL.iccInfoPrivate.PNN = pnn;
      }
    }

    let pnn = [];
    ICCIOHelper.loadLinearFixedEF({fileId: ICC_EF_PNN,
                                   callback: callback.bind(this)});
  },

  






  getADNFreeRecordId: function getADNFreeRecordId(fileId, onsuccess, onerror) {
    function callback(options) {
      let contact = GsmPDUHelper.readAlphaIdDiallingNumber(options.recordSize);
      if (!contact) {
        
        if (onsuccess) {
          onsuccess(options.p1);
        }
        return;
      }

      if (options.p1 < options.totalRecords) {
        ICCIOHelper.loadNextRecord(options);
      } else {
        
        if (onerror) {
          onerror("No free record found.");
        }
      }
    }

    ICCIOHelper.loadLinearFixedEF({fileId: fileId,
                                   callback: callback.bind(this),
                                   onerror: onerror});
  },

  








  readPLMNEntries: function readPLMNEntries(length) {
    let plmnList = [];
    
    debug("readPLMNEntries: PLMN entries length = " + length);
    let index = 0;
    while (index < length) {
      
      
      try {
        let plmn = [GsmPDUHelper.readHexOctet(),
                    GsmPDUHelper.readHexOctet(),
                    GsmPDUHelper.readHexOctet()];
        if (DEBUG) debug("readPLMNEntries: Reading PLMN entry: [" + index +
                         "]: '" + plmn + "'");
        if (plmn[0] != 0xFF &&
            plmn[1] != 0xFF &&
            plmn[2] != 0xFF) {
          let semiOctets = [];
          for (let idx = 0; idx < plmn.length; idx++) {
            semiOctets.push((plmn[idx] & 0xF0) >> 4);
            semiOctets.push(plmn[idx] & 0x0F);
          }

          
          
          
          
          
          
          let reformat = [semiOctets[1], semiOctets[0], semiOctets[3],
                          semiOctets[5], semiOctets[4], semiOctets[2]];
          let buf = "";
          let plmnEntry = {};
          for (let i = 0; i < reformat.length; i++) {
            if (reformat[i] != 0xF) {
              buf += GsmPDUHelper.semiOctetToBcdChar(reformat[i]);
            }
            if (i === 2) {
              
              plmnEntry.mcc = parseInt(buf);
              buf = "";
            } else if (i === 5) {
              
              plmnEntry.mnc = parseInt(buf);
            }
          }
          if (DEBUG) debug("readPLMNEntries: PLMN = " + plmnEntry.mcc + ", " + plmnEntry.mnc);
          plmnList.push(plmnEntry);
        }
      } catch (e) {
        if (DEBUG) debug("readPLMNEntries: PLMN entry " + index + " is invalid.");
        break;
      }
      index ++;
    }
    return plmnList;
  },
};




let ICCUtilsHelper = {
  









  getNetworkNameFromICC: function getNetworkNameFromICC(mcc, mnc, lac) {
    let iccInfoPriv = RIL.iccInfoPrivate;
    let iccInfo = RIL.iccInfo;
    let pnnEntry;

    if (!mcc || !mnc || !lac) {
      return null;
    }

    
    if (!iccInfoPriv.PNN) {
      return null;
    }

    
    
    
    
    let length = iccInfoPriv.OPL ? iccInfoPriv.OPL.length : 0;
    for (let i = 0; i < length; i++) {
      let opl = iccInfoPriv.OPL[i];
      
      if (mcc != opl.mcc || mnc != opl.mnc) {
        continue;
      }
      
      
      
      if ((opl.lacTacStart == 0x0 && opl.lacTacEnd == 0xFFFE) ||
          (opl.lacTacStart <= lac && opl.lacTacEnd >= lac)) {
        if (opl.pnnRecordId == 0) {
          
          
          
          return null;
        }
        pnnEntry = iccInfoPriv.PNN[opl.pnnRecordId - 1]
        break;
      }
    }

    
    
    
    
    
    if (!pnnEntry && mcc == iccInfo.mcc && mnc == iccInfo.mnc) {
      pnnEntry = iccInfoPriv.PNN[0]
    }

    if (!pnnEntry) {
      return null;
    }

    
    return {fullName: pnnEntry.fullName || "",
            shortName: pnnEntry.shortName || ""};
  },

  





  updateDisplayCondition: function updateDisplayCondition() {
    
    
    
    let iccInfo = RIL.iccInfo;
    let iccInfoPriv = RIL.iccInfoPrivate;
    let iccSpn = iccInfoPriv.SPN;
    let origIsDisplayNetworkNameRequired = iccInfo.isDisplayNetworkNameRequired;
    let origIsDisplaySPNRequired = iccInfo.isDisplaySpnRequired;

    if (!iccSpn) {
      iccInfo.isDisplayNetworkNameRequired = true;
      iccInfo.isDisplaySpnRequired = false;
    } else {
      let operatorMnc = RIL.operator.mnc;
      let operatorMcc = RIL.operator.mcc;

      
      
      let isOnMatchingPlmn = false;

      
      
      if (iccInfo.mcc == operatorMcc && iccInfo.mnc == operatorMnc) {
        isOnMatchingPlmn = true;
      }

      
      if (!isOnMatchingPlmn && iccInfoPriv.SPDI) {
        let iccSpdi = iccInfoPriv.SPDI; 
        for (let plmn in iccSpdi) {
          let plmnMcc = iccSpdi[plmn].mcc;
          let plmnMnc = iccSpdi[plmn].mnc;
          isOnMatchingPlmn = (plmnMcc == operatorMcc) && (plmnMnc == operatorMnc);
          if (isOnMatchingPlmn) {
            break;
          }
        }
      }

      if (isOnMatchingPlmn) {
        
        
        if (DEBUG) debug("updateDisplayCondition: PLMN is HPLMN or PLMN is in PLMN list");

        
        
        
        iccInfo.isDisplaySpnRequired = true;
        if (iccSpn.spnDisplayCondition & 0x01) {
          iccInfo.isDisplayNetworkNameRequired = true;
        } else {
          iccInfo.isDisplayNetworkNameRequired = false;
        }
      } else {
        
        
        if (DEBUG) debug("updateICCDisplayName: PLMN isn't HPLMN and PLMN isn't in PLMN list");

        
        
        
        if (iccSpn.spnDisplayCondition & 0x02) {
          iccInfo.isDisplayNetworkNameRequired = false;
          iccInfo.isDisplaySpnRequired = false;
        } else {
          iccInfo.isDisplayNetworkNameRequired = false;
          iccInfo.isDisplaySpnRequired = true;
        }
      }
    }

    if (DEBUG) {
      debug("updateDisplayCondition: isDisplayNetworkNameRequired = " + iccInfo.isDisplayNetworkNameRequired);
      debug("updateDisplayCondition: isDisplaySpnRequired = " + iccInfo.isDisplaySpnRequired);
    }

    return ((origIsDisplayNetworkNameRequired !== iccInfo.isDisplayNetworkNameRequired) ||
            (origIsDisplaySPNRequired !== iccInfo.isDisplaySpnRequired));
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

  


  parsePbrTlvs: function parsePbrTlvs(pbrTlvs) {
    let pbr = {};
    for (let i = 0; i < pbrTlvs.length; i++) {
      let pbrTlv = pbrTlvs[i];
      for (let j = 0; j < pbrTlv.value.length; j++) {
        let tlv = pbrTlv.value[j];
        let tagName = USIM_TAG_NAME[tlv.tag];

        
        if (tlv.tag == ICC_USIM_EFANR_TAG) {
          if (!pbr.anr) {
            pbr.anr = [];
          }
          pbr.anr.push(tlv);
        } else {
          pbr[tagName] = tlv;
        }

        pbr[tagName].fileType = pbrTlv.tag;
        pbr[tagName].fileId = (tlv.value[0] << 8) | tlv.value[1];

        
        if (pbrTlv.tag == ICC_USIM_TYPE2_TAG) {
          pbr[tagName].indexInIAP = j;
        }
      }
    }

    return pbr;
  },

  


  handleICCInfoChange: function handleICCInfoChange() {
    RIL.iccInfo.rilMessageType = "iccinfochange";
    RIL.sendDOMMessage(RIL.iccInfo);
  },

  







  isICCServiceAvailable: function isICCServiceAvailable(geckoService) {
    let serviceTable = RIL._isCdma ? RIL.iccInfoPrivate.cst:
                                     RIL.iccInfoPrivate.sst;
    let index, bitmask;
    if (RIL.appType == CARD_APPTYPE_SIM || RIL.appType == CARD_APPTYPE_RUIM) {
      













      let simService;
      if (RIL.appType == CARD_APPTYPE_SIM) {
        simService = GECKO_ICC_SERVICES.sim[geckoService];
      } else {
        simService = GECKO_ICC_SERVICES.ruim[geckoService];
      }
      if (!simService) {
        return false;
      }
      simService -= 1;
      index = Math.floor(simService / 4);
      bitmask = 2 << ((simService % 4) << 1);
    } else if (RIL.appType == CARD_APPTYPE_USIM) {
      













      let usimService = GECKO_ICC_SERVICES.usim[geckoService];
      if (!usimService) {
        return false;
      }
      usimService -= 1;
      index = Math.floor(usimService / 8);
      bitmask = 1 << ((usimService % 8) << 0);
    }

    return (serviceTable &&
           (index < serviceTable.length) &&
           (serviceTable[index] & bitmask)) != 0;
  },

  





  isGsm8BitAlphabet: function isGsm8BitAlphabet(str) {
    if (!str) {
      return false;
    }

    const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
    const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];

    for (let i = 0; i < str.length; i++) {
      let c = str.charAt(i);
      let octet = langTable.indexOf(c);
      if (octet == -1) {
        octet = langShiftTable.indexOf(c);
        if (octet == -1) {
          return false;
        }
      }
    }

    return true;
  },
};




let ICCContactHelper = {
  







  readICCContacts: function readICCContacts(appType, contactType, onsuccess, onerror) {
    switch (contactType) {
      case "ADN":
        switch (appType) {
          case CARD_APPTYPE_SIM:
            this.readSimContacts(onsuccess, onerror);
            break;
          case CARD_APPTYPE_USIM:
            this.readUSimContacts(onsuccess, onerror);
            break;
        }
        break;
      case "FDN":
        ICCRecordHelper.readFDN(onsuccess, onerror);
        break;
    }
  },

  







  findFreeICCContact: function findFreeICCContact(appType, contactType, onsuccess, onerror) {
    switch (contactType) {
      case "ADN":
        switch (appType) {
          case CARD_APPTYPE_SIM:
            ICCRecordHelper.getADNFreeRecordId(ICC_EF_ADN, onsuccess, onerror);
            break;
          case CARD_APPTYPE_USIM:
            let gotPbrCb = function gotPbrCb(pbr) {
              if (pbr.adn) {
                ICCRecordHelper.getADNFreeRecordId(pbr.adn.fileId, onsuccess, onerror);
              }
            }.bind(this);

            ICCRecordHelper.readPBR(gotPbrCb, onerror);
            break;
        }
        break;
      default:
        if (onerror) {
          onerror(GECKO_ERROR_REQUEST_NOT_SUPPORTED);
        }
        break;
    }
  },

  








  addICCContact: function addICCContact(appType, contactType, contact, onsuccess, onerror) {
    let foundFreeCb = function foundFreeCb(recordId) {
      contact.recordId = recordId;
      ICCContactHelper.updateICCContact(appType, contactType, contact, onsuccess, onerror);
    }.bind(this);

    
    ICCContactHelper.findFreeICCContact(appType, contactType, foundFreeCb, onerror);
  },

  








  updateICCContact: function updateICCContact(appType, contactType, contact, onsuccess, onerror) {
    switch (contactType) {
      case "ADN":
        switch (appType) {
          case CARD_APPTYPE_SIM:
            this.updateSimContact(contact, onsuccess, onerror);
            break;
          case CARD_APPTYPE_USIM:
            this.updateUSimContact(contact, onsuccess, onerror);
            break;
        }
        break;
      default:
        if (onerror) {
          onerror(GECKO_ERROR_REQUEST_NOT_SUPPORTED);
        }
        break;
    }
  },

  





  readUSimContacts: function readUSimContacts(onsuccess, onerror) {
    let gotPbrCb = function gotPbrCb(pbr) {
      if (pbr.adn) {
        let gotAdnCb = function gotAdnCb(contacts) {
          
          if (!pbr.email) {
            if (onsuccess) {
              onsuccess(contacts)
            }
            return;
          }

          this.readUSimEmails(pbr, contacts, onsuccess, onerror);
        }.bind(this);

        let fileId = pbr.adn.fileId;
        ICCRecordHelper.readADN(fileId, gotAdnCb, onerror);
      } else {
        let error = onerror || debug;
        error("Cannot access ADN.");
      }
    }.bind(this);

    ICCRecordHelper.readPBR(gotPbrCb, onerror);
  },

  







  readUSimEmails: function readUSimEmails(pbr, contacts, onsuccess, onerror) {
    (function doReadContactEmail(n) {
      if (n >= contacts.length) {
        
        if (onsuccess) {
          onsuccess(contacts);
        }
        return;
      }

      
      ICCContactHelper.readUSimContactEmail(
        pbr, contacts[n], doReadContactEmail.bind(this, n + 1), onerror);
    })(0);
  },

  







  readUSimContactEmail: function readUSimContactEmail(pbr, contact, onsuccess, onerror) {
    let gotRecordIdCb = function gotRecordIdCb(recordId) {
      if (recordId == 0xff) {
        
        if (onsuccess) {
          onsuccess();
        }
        return;
      }

      let fileId = pbr.email.fileId;
      let fileType = pbr.email.fileType;
      let gotEmailCb = function gotEmailCb(email) {
        
        if (email) {
          contact.email = email;
        }
        if (onsuccess) {
          onsuccess();
        }
      }.bind(this);

      ICCRecordHelper.readEmail(fileId, fileType, recordId, gotEmailCb, onerror);
    }.bind(this);

    this.getEmailRecordId(pbr, contact, gotRecordIdCb, onerror);
  },

  





  readSimContacts: function readSimContacts(onsuccess, onerror) {
    ICCRecordHelper.readADN(ICC_EF_ADN, onsuccess, onerror);
  },

  






  updateUSimContact: function updateUSimContact(contact, onsuccess, onerror) {
    let gotPbrCb = function gotPbrCb(pbr) {
      if (pbr.adn) {
        ICCRecordHelper.updateADN(pbr.adn.fileId, contact, onsuccess, onerror);
      } else {
        if (onerror) {
          onerror("Cannot access ADN.");
        }
      }
    }.bind(this);

    ICCRecordHelper.readPBR(gotPbrCb, onerror);
  },

  






  updateSimContact: function updateSimContact(contact, onsuccess, onerror) {
    ICCRecordHelper.updateADN(ICC_EF_ADN, contact, onsuccess, onerror);
  },

  












  getEmailRecordId: function getEmailRecordId(pbr, contact, onsuccess, onerror) {
    if (pbr.email.fileType == ICC_USIM_TYPE1_TAG) {
      
      if (onsuccess) {
        onsuccess(contact.recordId);
      }
    } else {
      
      let gotIapCb = function gotIapCb(iap) {
        let indexInIAP = pbr.email.indexInIAP;
        let recordId = iap[indexInIAP];

        if (onsuccess) {
          onsuccess(recordId);
        }
      }.bind(this);

      let fileId = pbr.iap.fileId;
      ICCRecordHelper.readIAP(fileId, contact.recordId, gotIapCb, onerror);
    }
  },
};

let RuimRecordHelper = {
  fetchRuimRecords: function fetchRuimRecords() {
    ICCRecordHelper.getICCID();
    RIL.getIMSI();
    this.readCST();
    this.readCDMAHome();
  },

  



  readCDMAHome: function readCDMAHome() {
    function callback(options) {
      let strLen = Buf.readUint32();
      let tempOctet = GsmPDUHelper.readHexOctet();
      cdmaHomeSystemId.push(((GsmPDUHelper.readHexOctet() & 0x7f) << 8) | tempOctet);
      tempOctet = GsmPDUHelper.readHexOctet();
      cdmaHomeNetworkId.push(((GsmPDUHelper.readHexOctet() & 0xff) << 8) | tempOctet);

      
      Buf.seekIncoming(PDU_HEX_OCTET_SIZE);

      Buf.readStringDelimiter(strLen);
      if (options.p1 < options.totalRecords) {
        ICCIOHelper.loadNextRecord(options);
      } else {
        if (DEBUG) {
          debug("CDMAHome system id: " + JSON.stringify(cdmaHomeSystemId));
          debug("CDMAHome network id: " + JSON.stringify(cdmaHomeNetworkId));
        }
        RIL.cdmaHome = {
          systemId: cdmaHomeSystemId,
          networkId: cdmaHomeNetworkId
        };
      }
    }

    let cdmaHomeSystemId = [], cdmaHomeNetworkId = [];
    ICCIOHelper.loadLinearFixedEF({fileId: ICC_EF_CSIM_CDMAHOME,
                                   callback: callback.bind(this)});
  },

  



  readCST: function readCST() {
    function callback() {
      let strLen = Buf.readUint32();
      
      RIL.iccInfoPrivate.cst = GsmPDUHelper.readHexOctetArray(strLen / 2);
      Buf.readStringDelimiter(strLen);

      if (DEBUG) {
        let str = "";
        for (let i = 0; i < RIL.iccInfoPrivate.cst.length; i++) {
          str += RIL.iccInfoPrivate.cst[i] + ", ";
        }
        debug("CST: " + str);
      }
    }
    ICCIOHelper.loadTransparentEF({fileId: ICC_EF_CSIM_CST,
                                   callback: callback.bind(this)});
  }
};





if (!this.debug) {
  
  this.debug = function debug(message) {
    dump("RIL Worker[" + CLIENT_ID + "]: " + message + "\n");
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
