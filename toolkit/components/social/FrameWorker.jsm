












"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/MessagePortBase.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SocialService",
  "resource://gre/modules/SocialService.jsm");

this.EXPORTED_SYMBOLS = ["getFrameWorkerHandle"];

var workerCache = {}; 
var _nextPortId = 1;



this.getFrameWorkerHandle =
 function getFrameWorkerHandle(url, clientWindow, name, origin) {
  
  
  let portid = _nextPortId++;
  let clientPort = new ClientPort(portid, clientWindow);

  let existingWorker = workerCache[url];
  if (!existingWorker) {
    
    let worker = new FrameWorker(url, name, origin);
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










function FrameWorker(url, name, origin) {
  this.url = url;
  this.name = name || url;
  this.ports = new Map();
  this.pendingPorts = [];
  this.loaded = false;
  this.reloading = false;
  this.origin = origin;

  this.frame = makeHiddenFrame();
  this.load();
}

FrameWorker.prototype = {
  load: function FrameWorker_loadWorker() {
    var self = this;
    Services.obs.addObserver(function injectController(doc, topic, data) {
      if (!doc.defaultView || doc.defaultView != self.frame.contentWindow) {
        return;
      }
      Services.obs.removeObserver(injectController, "document-element-inserted");
      try {
        self.createSandbox();
      } catch (e) {
        Cu.reportError("FrameWorker: failed to create sandbox for " + url + ". " + e);
      }
    }, "document-element-inserted", false);

    this.frame.setAttribute("src", this.url);
  },

  reload: function FrameWorker_reloadWorker() {
    
    
    for (let [, port] of this.ports) {
      port._window = null;
      this.pendingPorts.push(port);
    }
    this.ports.clear();
    
    
    this.loaded = false;
    
    
    
    this.reloading = true;
    this.frame.setAttribute("src", "about:blank");
  },

  createSandbox: function createSandbox() {
    let workerWindow = this.frame.contentWindow;
    let sandbox = new Cu.Sandbox(workerWindow);

    
    
    
    let workerAPI = ['WebSocket', 'localStorage', 'atob', 'btoa',
                     'clearInterval', 'clearTimeout', 'dump',
                     'setInterval', 'setTimeout', 'XMLHttpRequest',
                     'FileReader', 'Blob',
                     'location'];
    
    
    let needsWaive = ['XMLHttpRequest', 'WebSocket'];
    
    let needsBind = ['atob', 'btoa', 'dump', 'setInterval', 'clearInterval',
                     'setTimeout', 'clearTimeout'];
    workerAPI.forEach(function(fn) {
      try {
        if (needsWaive.indexOf(fn) != -1)
          sandbox[fn] = XPCNativeWrapper.unwrap(workerWindow)[fn];
        else if (needsBind.indexOf(fn) != -1)
          sandbox[fn] = workerWindow[fn].bind(workerWindow);
        else
          sandbox[fn] = workerWindow[fn];
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

    
    
    
    sandbox._evalInSandbox = function(s) {
      Cu.evalInSandbox(s, sandbox);
    };

    
    
    workerWindow.addEventListener('offline', function fw_onoffline(event) {
      Cu.evalInSandbox("onoffline();", sandbox);
    }, false);
    workerWindow.addEventListener('online', function fw_ononline(event) {
      Cu.evalInSandbox("ononline();", sandbox);
    }, false);

    sandbox._postMessage = function fw_postMessage(d, o) {
      workerWindow.postMessage(d, o)
    };
    sandbox._addEventListener = function fw_addEventListener(t, l, c) {
      workerWindow.addEventListener(t, l, c)
    };

    
    
    
    let worker = this;

    workerWindow.addEventListener("DOMContentLoaded", function loadListener() {
      workerWindow.removeEventListener("DOMContentLoaded", loadListener);

      
      let scriptText = workerWindow.document.body.textContent.trim();
      if (!scriptText) {
        Cu.reportError("FrameWorker: Empty worker script received");
        notifyWorkerError(worker);
        return;
      }

      
      
      workerWindow.document.body.textContent = "";

      
      
      try {
        Services.scriptloader.loadSubScript("resource://gre/modules/MessagePortBase.jsm", sandbox);
        Services.scriptloader.loadSubScript("resource://gre/modules/MessagePortWorker.js", sandbox);
      }
      catch (e) {
        Cu.reportError("FrameWorker: Error injecting port code into content side of the worker: " + e + "\n" + e.stack);
        notifyWorkerError(worker);
        return;
      }

      
      try {
        initClientMessageHandler(worker, workerWindow);
      }
      catch (e) {
        Cu.reportError("FrameWorker: Error setting up event listener for chrome side of the worker: " + e + "\n" + e.stack);
        notifyWorkerError(worker);
        return;
      }

      
      try {
        Cu.evalInSandbox(scriptText, sandbox, "1.8", workerWindow.location.href, 1);
      } catch (e) {
        Cu.reportError("FrameWorker: Error evaluating worker script for " + worker.name + ": " + e + "; " +
            (e.lineNumber ? ("Line #" + e.lineNumber) : "") +
            (e.stack ? ("\n" + e.stack) : ""));
        notifyWorkerError(worker);
        return;
      }

      
      worker.loaded = true;
      for (let port of worker.pendingPorts) {
        try {
          port._createWorkerAndEntangle(worker);
        }
        catch(e) {
          Cu.reportError("FrameWorker: Failed to create worker port: " + e + "\n" + e.stack);
        }
      }
      worker.pendingPorts = [];
    });

    
    
    
    workerWindow.addEventListener("unload", function unloadListener() {
      workerWindow.removeEventListener("unload", unloadListener);
      for (let [, port] of worker.ports) {
        try {
          port.close();
        } catch (ex) {
          Cu.reportError("FrameWorker: failed to close port. " + ex);
        }
      }
      
      
      
      worker.ports.clear();
      
      
      worker.loaded = false;
      
      
      if (!worker.reloading) {
        for (let port of worker.pendingPorts) {
          try {
            port.close();
          } catch (ex) {
            Cu.reportError("FrameWorker: failed to close pending port. " + ex);
          }
        }
        worker.pendingPorts = [];
      }

      if (sandbox) {
        Cu.nukeSandbox(sandbox);
        sandbox = null;
      }
      if (worker.reloading) {
        Services.tm.mainThread.dispatch(function doReload() {
          worker.reloading = false;
          worker.load();
        }, Ci.nsIThread.DISPATCH_NORMAL);
      }
    });
  },

  terminate: function terminate() {
    if (!(this.url in workerCache)) {
      
      return;
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
  
  iframe.style.display = "none";

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
      
      case "port-connection-error":
        
        
        notifyWorkerError(worker);
        break;
      case "port-close":
        
        port = worker.ports.get(portid);
        if (!port) {
          
          
          
          return;
        }
        worker.ports.delete(portid);
        port.close();
        break;

      case "port-message":
        
        port = worker.ports.get(portid);
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
    worker.ports.set(this._portid, this);
    this._postControlMessage("port-create");
    for (let message of this._pendingMessagesOutgoing) {
      this._dopost(message);
    }
    this._pendingMessagesOutgoing = [];
    
    
    if (this._closed) {
      this._window = null;
      worker.ports.delete(this._portid);
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
    if (this._closed) {
      return; 
    }
    
    
    this.postMessage({topic: "social.port-closing"});
    AbstractPort.prototype.close.call(this);
    this._window = null;
    this._clientWindow = null;
    
    
  }
}

function notifyWorkerError(worker) {
  
  
  SocialService.getProvider(worker.origin, function (provider) {
    if (provider)
      provider.errorState = "frameworker-error";
    Services.obs.notifyObservers(null, "social:frameworker-error", worker.origin);
  });
}
