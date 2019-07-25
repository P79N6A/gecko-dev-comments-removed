












"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;
Cu.import("resource://gre/modules/Services.jsm");

const EXPORTED_SYMBOLS = ["getFrameWorkerHandle"];

var workerCache = {}; 
var _nextPortId = 1;



function getFrameWorkerHandle(url, clientWindow, name) {
  
  
  let portid = _nextPortId++;
  let clientPort = new ClientPort(portid, clientWindow);

  let existingWorker = workerCache[url];
  if (!existingWorker) {
    
    let worker = new FrameWorker(url, clientWindow, name);
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
      
      
      function getProtoSource(ob) {
        let raw = ob.prototype.toSource();
        return ob.name + ".prototype=" + raw + ";"
      }
      try {
        let scriptText = [importScripts.toSource(),
                          AbstractPort.toSource(),
                          getProtoSource(AbstractPort),
                          WorkerPort.toSource(),
                          getProtoSource(WorkerPort),
                          
                          "WorkerPort.prototype.__proto__=AbstractPort.prototype;",
                          __initWorkerMessageHandler.toSource(),
                          "__initWorkerMessageHandler();" 
                         ].join("\n")
        Cu.evalInSandbox(scriptText, sandbox, "1.8", "<injected port handling code>", 1);
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

  hiddenDoc.documentElement.appendChild(iframe);

  
  let docShell = iframe.contentWindow.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDocShell);
  docShell.allowAuth = false;
  docShell.allowPlugins = false;
  docShell.allowImages = false;
  docShell.allowWindowControl = false;
  
  
  
  docShell.isBrowserFrame = true;

  return iframe;
}

function WorkerHandle(port, worker) {
  this.port = port;
  this._worker = worker;
}
WorkerHandle.prototype = {
  __exposedProps__: {
    port: "r",
    terminate: "r"
  },

  
  
  
  
  terminate: function terminate() {
    this._worker.terminate();
  }
};



function __initWorkerMessageHandler() {

  let ports = {}; 

  function messageHandler(event) {
    
    let data = event.data;
    let portid = data.portId;
    let port;
    if (!data.portFromType || data.portFromType === "worker") {
      
      return;
    }
    switch (data.portTopic) {
      case "port-create":
        
        
        port = new WorkerPort(portid);
        ports[portid] = port;
        
        onconnect({ports: [port]});
        break;

      case "port-close":
        
        port = ports[portid];
        if (!port) {
          
          
          
          return;
        }
        delete ports[portid];
        port.close();
        break;

      case "port-message":
        
        port = ports[portid];
        if (!port) {
          
          return;
        }
        port._onmessage(data.data);
        break;

      default:
        break;
    }
  }
  
  _addEventListener('message', messageHandler);
}


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



function AbstractPort(portid) {
  this._portid = portid;
  this._handler = undefined;
  
  this._pendingMessagesIncoming = [];
}

AbstractPort.prototype = {
  _portType: null, 
  
  _dopost: function fw_AbstractPort_dopost(data) {
    throw new Error("not implemented");
  },
  _onerror: function fw_AbstractPort_onerror(err) {
    throw new Error("not implemented");
  },

  
  toString: function fw_AbstractPort_toString() {
    return "MessagePort(portType='" + this._portType + "', portId=" + this._portid + ")";
  },
  _JSONParse: function fw_AbstractPort_JSONParse(data) JSON.parse(data),

 _postControlMessage: function fw_AbstractPort_postControlMessage(topic, data) {
    let postData = {portTopic: topic,
                    portId: this._portid,
                    portFromType: this._portType,
                    data: data,
                    __exposedProps__: {
                      portTopic: 'r',
                      portId: 'r',
                      portFromType: 'r',
                      data: 'r'
                    }
                   };
    this._dopost(postData);
  },

  _onmessage: function fw_AbstractPort_onmessage(data) {
    
    
    
    
    
    data = this._JSONParse(data);
    if (!this._handler) {
      this._pendingMessagesIncoming.push(data);
    }
    else {
      try {
        this._handler({data: data,
                       __exposedProps__: {data: 'r'}
                      });
      }
      catch (ex) {
        this._onerror(ex);
      }
    }
  },

  set onmessage(handler) { 
    this._handler = handler;
    while (this._pendingMessagesIncoming.length) {
      this._onmessage(this._pendingMessagesIncoming.shift());
    }
  },

  







  postMessage: function fw_AbstractPort_postMessage(data) {
    if (this._portid === null) {
      throw new Error("port is closed");
    }
    
    
    
    
    
    
    
    this._postControlMessage("port-message", JSON.stringify(data));
  },

  close: function fw_AbstractPort_close() {
    if (!this._portid) {
      return; 
    }
    this._postControlMessage("port-close");
    
    this._handler = null;
    this._pendingMessagesIncoming = [];
    this._portid = null;
  }
}




function WorkerPort(portid) {
  AbstractPort.call(this, portid);
}

WorkerPort.prototype = {
  __proto__: AbstractPort.prototype,
  _portType: "worker",

  _dopost: function fw_WorkerPort_dopost(data) {
    
    _postMessage(data, "*");
  },

  _onerror: function fw_WorkerPort_onerror(err) {
    throw new Error("Port " + this + " handler failed: " + err);
  }
}











function ClientPort(portid, clientWindow) {
  this._clientWindow = clientWindow
  this._window = null;
  
  this._pendingMessagesOutgoing = [];
  AbstractPort.call(this, portid);
}

ClientPort.prototype = {
  __exposedProps__: {
    'port': 'r',
    'onmessage': 'rw',
    'postMessage': 'r',
    'close': 'r'
  },
  __proto__: AbstractPort.prototype,
  _portType: "client",

  _JSONParse: function fw_ClientPort_JSONParse(data) {
    if (this._clientWindow) {
      return this._clientWindow.JSON.parse(data);
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
    this._pendingMessagesOutgoing = null;
  }
}


function importScripts() {
  for (var i=0; i < arguments.length; i++) {
    
    var scriptURL = arguments[i];
    var xhr = new XMLHttpRequest();
    xhr.open('GET', scriptURL, false);
    xhr.onreadystatechange = function(aEvt) {
      if (xhr.readyState == 4) {
        if (xhr.status == 200 || xhr.status == 0) {
          eval(xhr.responseText);
        }
        else {
          throw new Error("Unable to importScripts ["+scriptURL+"], status " + xhr.status)
        }
      }
    };
    xhr.send(null);
  }
}
