












"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/MessagePortBase.jsm");

const EXPORTED_SYMBOLS = ["getFrameWorkerHandle"];

var workerCache = {}; 
var _nextPortId = 1;



function getFrameWorkerHandle(url, clientWindow, name) {
  
  
  let portid = _nextPortId++;
  let clientPort = new ClientPort(portid, clientWindow);

  let existingWorker = workerCache[url];
  if (!existingWorker) {
    
    let worker = new FrameWorker(url, name);
    worker.pendingPorts.push(clientPort);
    existingWorker = workerCache[url] = worker;
  } else {
    
    if (existingWorker.loaded) {
      try {
        clientPort._createWorkerAndEntangle(existingWorker);
      }
      catch (ex) {
        Cu.reportError("FrameWorker: Failed to connect a port: " + e + "\n" + e.stack);
      }
    } else {
      existingWorker.pendingPorts.push(clientPort);
    }
  }

  
  return new WorkerHandle(clientPort, existingWorker);
};










function FrameWorker(url, name) {
  this.url = url;
  this.name = name || url;
  this.ports = {};
  this.pendingPorts = [];
  this.loaded = false;

  this.frame = makeHiddenFrame();

  var self = this;
  Services.obs.addObserver(function injectController(doc, topic, data) {
    if (!doc.defaultView || doc.defaultView != self.frame.contentWindow) {
      return;
    }
    Services.obs.removeObserver(injectController, "document-element-inserted", false);
    try {
      self.createSandbox();
    } catch (e) {
      Cu.reportError("FrameWorker: failed to create sandbox for " + url + ". " + e);
    }
  }, "document-element-inserted", false);

  this.frame.setAttribute("src", url);
}

FrameWorker.prototype = {
  createSandbox: function createSandbox() {
    let workerWindow = this.frame.contentWindow;
    let sandbox = new Cu.Sandbox(workerWindow);

    
    
    
    let workerAPI = ['MozWebSocket', 'WebSocket', 'localStorage',
                     'atob', 'btoa', 'clearInterval', 'clearTimeout', 'dump',
                     'setInterval', 'setTimeout', 'XMLHttpRequest',
                     'MozBlobBuilder', 'FileReader', 'Blob',
                     'location'];
    workerAPI.forEach(function(fn) {
      try {
        
        sandbox[fn] = XPCNativeWrapper.unwrap(workerWindow)[fn];
      }
      catch(e) {
        Cu.reportError("FrameWorker: failed to import API "+fn+"\n"+e+"\n");
      }
    });
    
    
    let navigator = {
      __exposedProps__: {
        "appName": "r",
        "appVersion": "r",
        "platform": "r",
        "userAgent": "r",
        "onLine": "r"
      },
      
      appName: workerWindow.navigator.appName,
      appVersion: workerWindow.navigator.appVersion,
      platform: workerWindow.navigator.platform,
      userAgent: workerWindow.navigator.userAgent,
      
      get onLine() workerWindow.navigator.onLine
    };
    sandbox.navigator = navigator;

    
    
    this.frame.addEventListener('offline', function fw_onoffline(event) {
      Cu.evalInSandbox("onoffline();", sandbox);
    }, false);
    this.frame.addEventListener('online', function fw_ononline(event) {
      Cu.evalInSandbox("ononline();", sandbox);
    }, false);

    sandbox._postMessage = function fw_postMessage(d, o) {
      workerWindow.postMessage(d, o)
    };
    sandbox._addEventListener = function fw_addEventListener(t, l, c) {
      workerWindow.addEventListener(t, l, c)
    };

    
    sandbox.bufferToArrayHack = function fw_bufferToArrayHack(a) {
      return new workerWindow.Uint8Array(a);
    };

    this.sandbox = sandbox;

    let worker = this;

    workerWindow.addEventListener("load", function loadListener() {
      workerWindow.removeEventListener("load", loadListener);
      
      
      try {
        Services.scriptloader.loadSubScript("resource://gre/modules/MessagePortBase.jsm", sandbox);
        Services.scriptloader.loadSubScript("resource://gre/modules/MessagePortWorker.js", sandbox);
      }
      catch (e) {
        Cu.reportError("FrameWorker: Error injecting port code into content side of the worker: " + e + "\n" + e.stack);
      }

      
      try {
        initClientMessageHandler(worker, workerWindow);
      }
      catch (e) {
        Cu.reportError("FrameWorker: Error setting up event listener for chrome side of the worker: " + e + "\n" + e.stack);
      }

      
      try {
        let scriptText = workerWindow.document.body.textContent;
        Cu.evalInSandbox(scriptText, sandbox, "1.8", workerWindow.location.href, 1);
      } catch (e) {
        Cu.reportError("FrameWorker: Error evaluating worker script for " + worker.name + ": " + e + "; " +
            (e.lineNumber ? ("Line #" + e.lineNumber) : "") +
            (e.stack ? ("\n" + e.stack) : ""));
        return;
      }

      
      worker.loaded = true;

      let pending = worker.pendingPorts;
      while (pending.length) {
        let port = pending.shift();
        if (port._portid) { 
          try {
            port._createWorkerAndEntangle(worker);
          }
          catch(e) {
            Cu.reportError("FrameWorker: Failed to create worker port: " + e + "\n" + e.stack);
          }
        }
      }
    });
  },

  terminate: function terminate() {
    
    for (let [portid, port] in Iterator(this.ports)) {
      
      if (!port)
        continue;
      try {
        port.close();
      } catch (ex) {
        Cu.reportError("FrameWorker: failed to close port. " + ex);
      }
    }

    delete workerCache[this.url];

    
    Services.tm.mainThread.dispatch(function deleteWorkerFrame() {
      
      this.frame.parentNode.removeChild(this.frame);
    }.bind(this), Ci.nsIThread.DISPATCH_NORMAL);
  }
};

function makeHiddenFrame() {
  let hiddenDoc = Services.appShell.hiddenDOMWindow.document;
  let iframe = hiddenDoc.createElementNS("http://www.w3.org/1999/xhtml", "iframe");
  iframe.setAttribute("mozframetype", "content");
  
  iframe.setAttribute("sandbox", "allow-same-origin");

  hiddenDoc.documentElement.appendChild(iframe);

  
  let docShell = iframe.contentWindow.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDocShell);
  docShell.allowAuth = false;
  docShell.allowPlugins = false;
  docShell.allowImages = false;
  docShell.allowWindowControl = false;
  
  return iframe;
}

function WorkerHandle(port, worker) {
  this.port = port;
  this._worker = worker;
}
WorkerHandle.prototype = {
  get document() {
    return this._worker.frame.contentDocument;
  },

  
  
  
  
  terminate: function terminate() {
    this._worker.terminate();
  }
};


function initClientMessageHandler(worker, workerWindow) {
  function _messageHandler(event) {
    
    let data = event.data;
    let portid = data.portId;
    let port;
    if (!data.portFromType || data.portFromType === "client") {
      
      return;
    }
    switch (data.portTopic) {
      

      case "port-close":
        
        port = worker.ports[portid];
        if (!port) {
          
          
          
          return;
        }
        delete worker.ports[portid];
        port.close();
        break;

      case "port-message":
        
        port = worker.ports[portid];
        if (!port) {
          return;
        }
        port._onmessage(data.data);
        break;

      default:
        break;
    }
  }
  
  function messageHandler(event) {
    try {
      _messageHandler(event);
    } catch (ex) {
      Cu.reportError("FrameWorker: Error handling client port control message: " + ex + "\n" + ex.stack);
    }
  }
  workerWindow.addEventListener('message', messageHandler);
}











function ClientPort(portid, clientWindow) {
  this._clientWindow = clientWindow;
  this._window = null;
  
  this._pendingMessagesOutgoing = [];
  AbstractPort.call(this, portid);
}

ClientPort.prototype = {
  __exposedProps__: {
    onmessage: "rw",
    postMessage: "r",
    close: "r",
    toString: "r"
  },
  __proto__: AbstractPort.prototype,
  _portType: "client",

  _JSONParse: function fw_ClientPort_JSONParse(data) {
    if (this._clientWindow) {
      return XPCNativeWrapper.unwrap(this._clientWindow).JSON.parse(data);
    }
    return JSON.parse(data);
  },

  _createWorkerAndEntangle: function fw_ClientPort_createWorkerAndEntangle(worker) {
    this._window = worker.frame.contentWindow;
    worker.ports[this._portid] = this;
    this._postControlMessage("port-create");
    while (this._pendingMessagesOutgoing.length) {
      this._dopost(this._pendingMessagesOutgoing.shift());
    }
  },

  _dopost: function fw_ClientPort_dopost(data) {
    if (!this._window) {
      this._pendingMessagesOutgoing.push(data);
    } else {
      this._window.postMessage(data, "*");
    }
  },

  _onerror: function fw_ClientPort_onerror(err) {
    Cu.reportError("FrameWorker: Port " + this + " handler failed: " + err + "\n" + err.stack);
  },

  close: function fw_ClientPort_close() {
    if (!this._portid) {
      return; 
    }
    
    
    this.postMessage({topic: "social.port-closing"});
    AbstractPort.prototype.close.call(this);
    this._window = null;
    this._clientWindow = null;
    this._pendingMessagesOutgoing = null;
  }
}
