


"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;


let subscriptLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
                        .getService(Ci.mozIJSSubScriptLoader);













function newWorker(custom_ns) {
  let worker_ns = {
    importScripts: function() {
      Array.slice(arguments).forEach(function(script) {
        if (!script.startsWith("resource:")) {
          script = "resource://gre/modules/" + script;
        }
        subscriptLoader.loadSubScript(script, this);
      }, this);
    },

    postRILMessage: function(message) {
    },

    postMessage: function(message) {
    },

    
    
    onmessage: undefined,
    onerror: undefined,

    DEBUG: true
  };
  
  worker_ns.self = worker_ns;

  
  for (let key in custom_ns) {
    worker_ns[key] = custom_ns[key];
  }

  
  let require = (function() {
    return function require(script) {
      worker_ns.module = {};
      worker_ns.importScripts(script);
      return worker_ns;
    }
  })();

  Object.freeze(require);
  Object.defineProperty(worker_ns, "require", {
    value: require,
    enumerable: true,
    configurable: false
  });

  
  worker_ns.importScripts("ril_worker.js");

  
  worker_ns.ContextPool.registerClient({ clientId: 0 });

  return worker_ns;
}






function newUint8Worker() {
  let worker = newWorker();
  let index = 0; 
  let buf = [];

  let context = worker.ContextPool._contexts[0];
  context.Buf.writeUint8 = function(value) {
    buf.push(value);
  };

  context.Buf.readUint8 = function() {
    return buf[index++];
  };

  context.Buf.seekIncoming = function(offset) {
    index += offset;
  };

  context.Buf.getReadAvailable = function() {
    return buf.length - index;
  };

  worker.debug = do_print;

  return worker;
}




function newInterceptWorker() {
  let postedMessage;
  let worker = newWorker({
    postRILMessage: function(data) {
    },
    postMessage: function(message) {
      postedMessage = message;
    }
  });
  return {
    get postedMessage() {
      return postedMessage;
    },
    get worker() {
      return worker;
    }
  };
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

  function writeInt32(value) {
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

  writeInt32(response);
  writeInt32(request);

  
  for (let ii = 0; ii < data.length; ++ii) {
    writeUint8(data[ii]);
  }

  return bytes;
}
