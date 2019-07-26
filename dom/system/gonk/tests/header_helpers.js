


"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;


let subscriptLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
                        .getService(Ci.mozIJSSubScriptLoader);













function newWorker(custom_ns) {
  let worker_ns = {
    importScripts: function fakeImportScripts() {
      Array.slice(arguments).forEach(function (script) {
        subscriptLoader.loadSubScript("resource://gre/modules/" + script, this);
      }, this);
    },

    postRILMessage: function fakePostRILMessage(message) {
    },

    postMessage: function fakepostMessage(message) {
    },

    
    
    onmessage: undefined,
    onerror: undefined,

    CLIENT_ID: 0,
    DEBUG: true
  };
  
  worker_ns.self = worker_ns;

  
  Cu.import("resource://gre/modules/ctypes.jsm", worker_ns);

  
  for (let key in custom_ns) {
    worker_ns[key] = custom_ns[key];
  }

  
  worker_ns.importScripts("ril_worker.js");

  return worker_ns;
}

















function newIncomingParcel(fakeParcelSize, response, request, data) {
  const UINT32_SIZE = 4;
  const PARCEL_SIZE_SIZE = 4;

  let realParcelSize = data.length + 2 * UINT32_SIZE;
  let buffer = new ArrayBuffer(realParcelSize + PARCEL_SIZE_SIZE);
  let bytes = new Uint8Array(buffer);

  let writeIndex = 0;
  function writeUint8(value) {
    bytes[writeIndex] = value;
    ++writeIndex;
  }

  function writeUint32(value) {
    writeUint8(value & 0xff);
    writeUint8((value >> 8) & 0xff);
    writeUint8((value >> 16) & 0xff);
    writeUint8((value >> 24) & 0xff);
  }

  function writeParcelSize(value) {
    writeUint8((value >> 24) & 0xff);
    writeUint8((value >> 16) & 0xff);
    writeUint8((value >> 8) & 0xff);
    writeUint8(value & 0xff);
  }

  if (fakeParcelSize < 0) {
    fakeParcelSize = realParcelSize;
  }
  writeParcelSize(fakeParcelSize);

  writeUint32(response);
  writeUint32(request);

  
  for (let ii = 0; ii < data.length; ++ii) {
    writeUint8(data[ii]);
  }

  return bytes;
}




function newRadioInterfaceLayer() {
  let ril_ns = {
    ChromeWorker: function ChromeWorker() {
      
    },
  };

  subscriptLoader.loadSubScript("resource://gre/components/RadioInterfaceLayer.js", ril_ns);
  return new ril_ns.RadioInterfaceLayer();
}













function do_check_throws(func, message, stack)
{
  if (!stack)
    stack = Components.stack.caller;

  try {
    func();
  } catch (exc) {
    if (exc.message === message) {
      return;
    }
    do_throw("expecting exception '" + message
             + "', caught '" + exc.message + "'", stack);
  }

  if (message) {
    do_throw("expecting exception '" + message + "', none thrown", stack);
  }
}

