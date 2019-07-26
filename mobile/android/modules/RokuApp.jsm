




"use strict";

this.EXPORTED_SYMBOLS = ["RokuApp"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");

function log(msg) {
  
}

const PROTOCOL_VERSION = 1;





function RokuApp(service) {
  this.service = service;
  this.resourceURL = this.service.location;
#ifdef RELEASE_BUILD
  this.app = "Firefox";
#else
  this.app = "Firefox Nightly";
#endif
  this.appID = -1;
}

RokuApp.prototype = {
  status: function status(callback) {
    
    
    let url = this.resourceURL + "query/apps";
    let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Ci.nsIXMLHttpRequest);
    xhr.open("GET", url, true);
    xhr.channel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;
    xhr.overrideMimeType("text/xml");

    xhr.addEventListener("load", (function() {
      if (xhr.status == 200) {
        let doc = xhr.responseXML;
        let apps = doc.querySelectorAll("app");
        for (let app of apps) {
          if (app.textContent == this.app) {
            this.appID = app.id;
          }
        }
      }

      
      if (callback) {
        callback({ state: "unknown" });
      }
    }).bind(this), false);

    xhr.addEventListener("error", (function() {
      if (callback) {
        callback({ state: "unknown" });
      }
    }).bind(this), false);

    xhr.send(null);
  },

  start: function start(callback) {
    
    if (this.appID == -1) {
      this.status(function() {
        
        if (this.appID != -1) {
          this.start(callback);
        } else {
          
          callback(false);
        }
      }.bind(this));
      return;
    }

    
    
    let url = this.resourceURL + "launch/" + this.appID + "?version=" + parseInt(PROTOCOL_VERSION);
    let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Ci.nsIXMLHttpRequest);
    xhr.open("POST", url, true);
    xhr.overrideMimeType("text/plain");

    xhr.addEventListener("load", (function() {
      if (callback) {
        callback(xhr.status === 200);
      }
    }).bind(this), false);

    xhr.addEventListener("error", (function() {
      if (callback) {
        callback(false);
      }
    }).bind(this), false);

    xhr.send(null);
  },

  stop: function stop(callback) {
    
    
    let url = this.resourceURL + "keypress/Home";
    let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Ci.nsIXMLHttpRequest);
    xhr.open("POST", url, true);
    xhr.overrideMimeType("text/plain");

    xhr.addEventListener("load", (function() {
      if (callback) {
        callback(xhr.status === 200);
      }
    }).bind(this), false);

    xhr.addEventListener("error", (function() {
      if (callback) {
        callback(false);
      }
    }).bind(this), false);

    xhr.send(null);
  },

  remoteMedia: function remoteMedia(callback, listener) {
    if (this.appID != -1) {
      if (callback) {
        callback(new RemoteMedia(this.resourceURL, listener));
      }
    } else {
      if (callback) {
        callback();
      }
    }
  }
}




function RemoteMedia(url, listener) {
  this._url = url;
  this._listener = listener;
  this._status = "uninitialized";

  let serverURI = Services.io.newURI(this._url , null, null);
  this._socket = Cc["@mozilla.org/network/socket-transport-service;1"].getService(Ci.nsISocketTransportService).createTransport(null, 0, serverURI.host, 9191, null);
  this._outputStream = this._socket.openOutputStream(0, 0, 0);

  this._scriptableStream = Cc["@mozilla.org/scriptableinputstream;1"].createInstance(Ci.nsIScriptableInputStream);

  this._inputStream = this._socket.openInputStream(0, 0, 0);
  this._pump = Cc["@mozilla.org/network/input-stream-pump;1"].createInstance(Ci.nsIInputStreamPump);
  this._pump.init(this._inputStream, -1, -1, 0, 0, true);
  this._pump.asyncRead(this, null);
}

RemoteMedia.prototype = {
  onStartRequest: function(request, context) {
  },

  onDataAvailable: function(request, context, stream, offset, count) {
    this._scriptableStream.init(stream);
    let data = this._scriptableStream.read(count);
    if (!data) {
      return;
    }

    let msg = JSON.parse(data);
    if (this._status === msg._s) {
      return;
    }

    this._status = msg._s;

    if (this._listener) {
      
      if (this._status == "connected" && "onRemoteMediaStart" in this._listener) {
        this._listener.onRemoteMediaStart(this);
      }

      if ("onRemoteMediaStatus" in this._listener) {
        this._listener.onRemoteMediaStatus(this);
      }
    }
  },

  onStopRequest: function(request, context, result) {
    if (this._listener && "onRemoteMediaStop" in this._listener)
      this._listener.onRemoteMediaStop(this);
  },

  _sendMsg: function _sendMsg(data) {
    if (!data)
      return;

    
    data["_v"] = PROTOCOL_VERSION;

    let raw = JSON.stringify(data);
    this._outputStream.write(raw, raw.length);
  },

  shutdown: function shutdown() {
    this._outputStream.close();
    this._inputStream.close();
  },

  get active() {
    return (this._socket && this._socket.isAlive());
  },

  play: function play() {
    
    this._sendMsg({ type: "PLAY" });
  },

  pause: function pause() {
    this._sendMsg({ type: "STOP" });
  },

  load: function load(aData) {
    this._sendMsg({ type: "LOAD", title: aData.title, source: aData.source, poster: aData.poster });
  },

  get status() {
    return this._status;
  }
}
