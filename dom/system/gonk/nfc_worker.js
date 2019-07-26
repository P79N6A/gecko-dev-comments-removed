
















"use strict";

importScripts("systemlibs.js", "nfc_consts.js");
importScripts("resource://gre/modules/workers/require.js");


let DEBUG = DEBUG_WORKER;

function getPaddingLen(len) {
  return (len % 4) ? (4 - len % 4) : 0;
}

let Buf = {
  __proto__: (function(){
    return require("resource://gre/modules/workers/worker_buf.js").Buf;
  })(),

  init: function init() {
    this._init();
  },

  


  processParcel: function processParcel() {
    let pduType = this.readInt32();
    if (DEBUG) debug("Number of bytes available in Parcel : " + this.readAvailable);
    NfcWorker.handleParcel(pduType, this.mCallback);
  },

  






  newParcel: function newParcel(type, callback) {
    if (DEBUG) debug("New outgoing parcel of type " + type);
    this.mCallback = callback;
    
    this.outgoingIndex = this.PARCEL_SIZE_SIZE;
    this.writeInt32(type);
  },

  simpleRequest: function simpleRequest(type) {
    this.newParcel(type);
    this.sendParcel();
  },

  onSendParcel: function onSendParcel(parcel) {
    postNfcMessage(parcel);
  },

  



  mCallback: null,
};






let NfcWorker = {
  





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

  


  unMarshallNdefMessage: function unMarshallNdefMessage() {
    let numOfRecords = Buf.readInt32();
    debug("numOfRecords = " + numOfRecords);
    if (numOfRecords <= 0) {
      return null;
    }
    let records = [];

    for (let i = 0; i < numOfRecords; i++) {
      let tnf        = Buf.readInt32() & 0xff;
      let typeLength = Buf.readInt32();
      let type       = Buf.readUint8Array(typeLength);
      let padding    = getPaddingLen(typeLength);
      for (let i = 0; i < padding; i++) {
        Buf.readUint8();
      }

      let idLength = Buf.readInt32();
      let id       = Buf.readUint8Array(idLength);
      padding      = getPaddingLen(idLength);
      for (let i = 0; i < padding; i++) {
        Buf.readUint8();
      }

      let payloadLength = Buf.readInt32();
      let payload       = Buf.readUint8Array(payloadLength);
      padding           = getPaddingLen(payloadLength);
      for (let i = 0; i < padding; i++) {
        Buf.readUint8();
      }
      records.push({tnf: tnf,
                    type: type,
                    id: id,
                    payload: payload});
    }
    return records;
  },

  


  readNDEF: function readNDEF(message) {
    let cb = function callback() {
      let error        = Buf.readInt32();
      let sessionId    = Buf.readInt32();
      let records      = this.unMarshallNdefMessage();

      message.type      = "ReadNDEFResponse";
      message.sessionId = sessionId;
      message.records   = records;
      message.status    = error;
      this.sendDOMMessage(message);
    }

    Buf.newParcel(NFC_REQUEST_READ_NDEF, cb);
    Buf.writeInt32(message.sessionId);
    Buf.sendParcel();
  },

  


  writeNDEF: function writeNDEF(message) {
    let cb = function callback() {
      let error         = Buf.readInt32();
      let sessionId     = Buf.readInt32();

      message.type      = "WriteNDEFResponse";
      message.sessionId = sessionId;
      message.status    = error;
      this.sendDOMMessage(message);
    };

    Buf.newParcel(NFC_REQUEST_WRITE_NDEF, cb);
    Buf.writeInt32(message.sessionId);
    let records    = message.records;
    let numRecords = records.length;
    Buf.writeInt32(numRecords);
    for (let i = 0; i < numRecords; i++) {
      let record = records[i];
      Buf.writeInt32(record.tnf);

      let typeLength = record.type ? record.type.length : 0;
      Buf.writeInt32(typeLength);
      for (let j = 0; j < typeLength; j++) {
        Buf.writeUint8(record.type[j]);
      }
      let padding = getPaddingLen(typeLength);
      for (let i = 0; i < padding; i++) {
        Buf.writeUint8(0x00);
      }

      let idLength = record.id ? record.id.length : 0;
      Buf.writeInt32(idLength);
      for (let j = 0; j < idLength; j++) {
        Buf.writeUint8(record.id[j]);
      }
      padding = getPaddingLen(idLength);
      for (let i = 0; i < padding; i++) {
        Buf.writeUint8(0x00);
      }

      let payloadLength = record.payload ? record.payload.length : 0;
      Buf.writeInt32(payloadLength);
      for (let j = 0; j < payloadLength; j++) {
        Buf.writeUint8(record.payload[j]);
      }
      padding = getPaddingLen(payloadLength);
      for (let i = 0; i < padding; i++) {
        Buf.writeUint8(0x00);
      }
    }

    Buf.sendParcel();
  },

  


  makeReadOnlyNDEF: function makeReadOnlyNDEF(message) {
    let cb = function callback() {
      let error         = Buf.readInt32();
      let sessionId     = Buf.readInt32();

      message.type      = "MakeReadOnlyNDEFResponse";
      message.sessionId = sessionId;
      message.status    = error;
      this.sendDOMMessage(message);
    };

    Buf.newParcel(NFC_REQUEST_MAKE_NDEF_READ_ONLY, cb);
    Buf.writeInt32(message.sessionId);
    Buf.sendParcel();
  },

  


  getDetailsNDEF: function getDetailsNDEF(message) {
    let cb = function callback() {
      let error                  = Buf.readInt32();
      let sessionId              = Buf.readInt32();
      let isReadOnly             = Buf.readUint8();
      let canBeMadeReadOnly      = Buf.readUint8();
      
      Buf.readUint8();
      Buf.readUint8();
      let maxSupportedLength     = Buf.readInt32();

      message.type               = "GetDetailsNDEFResponse";
      message.sessionId          = sessionId;
      message.isReadOnly         = isReadOnly;
      message.canBeMadeReadOnly  = canBeMadeReadOnly;
      message.maxSupportedLength = maxSupportedLength;
      message.status             = error;
      this.sendDOMMessage(message);
    };
    Buf.newParcel(NFC_REQUEST_GET_DETAILS, cb);
    Buf.writeInt32(message.sessionId);
    Buf.sendParcel();
  },


  


  connect: function connect(message) {
    let cb = function callback() {
      let error         = Buf.readInt32();
      let sessionId     = Buf.readInt32();

      message.type      = "ConnectResponse";
      message.sessionId = sessionId;
      message.status    = error;
      this.sendDOMMessage(message);
    };

    Buf.newParcel(NFC_REQUEST_CONNECT, cb);
    Buf.writeInt32(message.sessionId);
    Buf.writeInt32(message.techType);
    Buf.sendParcel();
  },

  


  config: function config(message) {
    let cb = function callback() {
      let error         = Buf.readInt32();

      message.type      = "ConfigResponse";
      message.status    = error;
      this.sendDOMMessage(message);
    };

    Buf.newParcel(NFC_REQUEST_CONFIG , cb);
    Buf.writeInt32(message.powerLevel);
    Buf.sendParcel();
  },

  


  close: function close(message) {
    let cb = function callback() {
      let error         = Buf.readInt32();
      let sessionId     = Buf.readInt32();

      message.type      = "CloseResponse";
      message.sessionId = sessionId;
      message.status    = error;
      this.sendDOMMessage(message);
    };

    Buf.newParcel(NFC_REQUEST_CLOSE , cb);
    Buf.writeInt32(message.sessionId);
    Buf.sendParcel();
  },

  handleParcel: function handleParcel(request_type, callback) {
    let method = this[request_type];
    if (typeof method == "function") {
      if (DEBUG) debug("Handling parcel as " + method.name);
      method.call(this);
    } else if (typeof callback == "function") {
      callback.call(this, request_type);
      this.mCallback = null;
    } else {
      debug("Unable to handle ReqType:"+request_type);
    }
  },

  


  sendDOMMessage: function sendDOMMessage(message) {
    postMessage(message);
  }
};




NfcWorker[NFC_NOTIFICATION_INITIALIZED] = function NFC_NOTIFICATION_INITIALIZED () {
  let status       = Buf.readInt32();
  let majorVersion = Buf.readInt32();
  let minorVersion = Buf.readInt32();
  debug("NFC_NOTIFICATION_INITIALIZED status:" + status);
  if ((majorVersion != NFC_MAJOR_VERSION) || (minorVersion != NFC_MINOR_VERSION)) {
    debug("Version Mismatch! Current Supported Version : " +
            NFC_MAJOR_VERSION + "." + NFC_MINOR_VERSION  +
           " Received Version : " + majorVersion + "." + minorVersion);
  }
};

NfcWorker[NFC_NOTIFICATION_TECH_DISCOVERED] = function NFC_NOTIFICATION_TECH_DISCOVERED() {
  debug("NFC_NOTIFICATION_TECH_DISCOVERED");
  let techList  = [];
  let records   = null;

  let sessionId = Buf.readInt32();
  let techCount = Buf.readInt32();
  for (let count = 0; count < techCount; count++) {
    let tech = NFC_TECHS[Buf.readUint8()];
    if (tech) {
      techList.push(tech);
    }
  }

  let padding   = getPaddingLen(techCount);
  for (let i = 0; i < padding; i++) {
    Buf.readUint8();
  }

  let ndefMsgCount = Buf.readInt32();
  if (ndefMsgCount > 0) {
    records = this.unMarshallNdefMessage();
  }
  this.sendDOMMessage({type: "techDiscovered",
                       sessionId: sessionId,
                       techList: techList,
                       records: records});
};

NfcWorker[NFC_NOTIFICATION_TECH_LOST] = function NFC_NOTIFICATION_TECH_LOST() {
  debug("NFC_NOTIFICATION_TECH_LOST");
  let sessionId = Buf.readInt32();
  debug("sessionId = " + sessionId);
  this.sendDOMMessage({type: "techLost",
                       sessionId: sessionId,
                       });
};





if (!this.debug) {
  
  this.debug = function debug(message) {
    dump("Nfc Worker: " + message + "\n");
  };
}



Buf.init();

function onNfcMessage(data) {
  Buf.processIncoming(data);
};

onmessage = function onmessage(event) {
  NfcWorker.handleDOMMessage(event.data);
};

onerror = function onerror(event) {
  debug("OnError: event: " + JSON.stringify(event));
  debug("NFC Worker error " + event.message + " " + event.filename + ":" +
        event.lineno + ":\n");
};
