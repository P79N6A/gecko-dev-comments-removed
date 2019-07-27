




"use strict";

this.EXPORTED_SYMBOLS = ["RokuApp"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");

const WEBRTC_PLAYER_NAME = "WebRTC Player";
const MIRROR_PORT = 8011;
const JSON_MESSAGE_TERMINATOR = "\r\n";

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
  this.mediaAppID = -1;
  this.mirrorAppID = -1;
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
            this.mediaAppID = app.id;
          } else if (app.textContent == WEBRTC_PLAYER_NAME) {
            this.mirrorAppID = app.id
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
    
    if (this.mediaAppID == -1) {
      this.status(function() {
        
        if (this.mediaAppID != -1) {
          this.start(callback);
        } else {
          
          callback(false);
        }
      }.bind(this));
      return;
    }

    
    
    let url = this.resourceURL + "launch/" + this.mediaAppID + "?version=" + parseInt(PROTOCOL_VERSION);
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
    if (this.mediaAppID != -1) {
      if (callback) {
        callback(new RemoteMedia(this.resourceURL, listener));
      }
    } else {
      if (callback) {
        callback();
      }
    }
  },

  mirror: function(callback, win, viewport, mirrorStartedCallback, contentWindow) {
    if (this.mirrorAppID == -1) {
      
      this.status(this._createRemoteMirror.bind(this, callback, win, viewport, mirrorStartedCallback, contentWindow));
    } else {
      this._createRemoteMirror(callback, win, viewport, mirrorStartedCallback, contentWindow);
    }
  },

  _createRemoteMirror: function(callback, win, viewport, mirrorStartedCallback, contentWindow) {
    if (this.mirrorAppID == -1) {
      
      log("RokuApp: Failed to find Mirror App ID.");
    } else {
      let url = this.resourceURL + "launch/" + this.mirrorAppID;
      let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Ci.nsIXMLHttpRequest);
      xhr.open("POST", url, true);
      xhr.overrideMimeType("text/plain");

      xhr.addEventListener("load", (function() {
        
        if ((xhr.status == 200) || (xhr.status == 204)) {
          this.remoteMirror = new RemoteMirror(this.resourceURL, win, viewport, mirrorStartedCallback, contentWindow);
        }
      }).bind(this), false);

      xhr.addEventListener("error", function() {
        log("RokuApp: XHR Failed to launch application: " + WEBRTC_PLAYER_NAME);
      }, false);

      xhr.send(null);
    }

    if (callback) {
      callback();
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

  load: function load(data) {
    this._sendMsg({ type: "LOAD", title: data.title, source: data.source, poster: data.poster });
  },

  get status() {
    return this._status;
  }
}

function RemoteMirror(url, win, viewport, mirrorStartedCallback, contentWindow) {
  this._serverURI = Services.io.newURI(url , null, null);
  this._window = win;
  this._iceCandidates = [];
  this.mirrorStarted = mirrorStartedCallback;

  
  
  let windowId = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
  let cWidth =  Math.max(viewport.cssWidth, viewport.width);
  let cHeight = Math.max(viewport.cssHeight, viewport.height);

  const MAX_WIDTH = 1280;
  const MAX_HEIGHT = 720;

  let tWidth = 0;
  let tHeight = 0;

  
  if ((cWidth / MAX_WIDTH) > (cHeight / MAX_HEIGHT)) {
    tHeight = Math.ceil((MAX_WIDTH / (4* cWidth)) * cHeight) * 4;
    tWidth = MAX_WIDTH;
  } else {
    tWidth = Math.ceil((MAX_HEIGHT / (4 * cHeight)) * cWidth) * 4;
    tHeight = MAX_HEIGHT;
  }

  let constraints = {
    video: {
      mediaSource: "browser",
      browserWindow: windowId,
      scrollWithPage: true,
      advanced: [
        {
          width: { min: tWidth, max: tWidth },
          height: { min: tHeight, max: tHeight }
        },
        { aspectRatio: cWidth / cHeight }
      ]
    }
  };

  this._window.navigator.mozGetUserMedia(constraints, this._onReceiveGUMStream.bind(this), function() {});
}

RemoteMirror.prototype = {
  _sendOffer: function(offer) {
    if (!this._baseSocket) {
      this._baseSocket = Cc["@mozilla.org/tcp-socket;1"].createInstance(Ci.nsIDOMTCPSocket);
    }
    this._jsonOffer = JSON.stringify(offer);
    this._socket = this._baseSocket.open(this._serverURI.host, MIRROR_PORT, { useSecureTransport: false, binaryType: "string" });
    this._socket.onopen = this._onSocketOpen.bind(this);
    this._socket.ondata = this._onSocketData.bind(this);
    this._socket.onerror = this._onSocketError.bind(this);
  },

  _onReceiveGUMStream: function(stream) {
    this._pc = new this._window.mozRTCPeerConnection;
    this._pc.addStream(stream);
    this._pc.onicecandidate = (evt => {
      
      if (!evt.candidate) {
        return;
      }
      let jsonCandidate = JSON.stringify(evt.candidate);
      this._iceCandidates.push(jsonCandidate);
      this._sendIceCandidates();
    });

    this._pc.createOffer(offer => {
      this._pc.setLocalDescription(
        new this._window.mozRTCSessionDescription(offer),
        () => this._sendOffer(offer),
        () => log("RemoteMirror: Failed to set local description."));
    },
    () => log("RemoteMirror: Failed to create offer."));
  },

  _stopMirror: function() {
    if (this._socket) {
      this._socket.close();
      this._socket = null;
    }
    if (this._pc) {
      this._pc.close();
      this._pc = null;
    }
    this._jsonOffer = null;
    this._iceCandidates = [];
  },

  _onSocketData: function(response) {
    if (response.type == "data") {
      response.data.split(JSON_MESSAGE_TERMINATOR).forEach(data => {
        if (data) {
          let parsedData = JSON.parse(data);
          if (parsedData.type == "answer") {
            this._pc.setRemoteDescription(
              new this._window.mozRTCSessionDescription(parsedData),
              () => this.mirrorStarted(this._stopMirror.bind(this)),
              () => log("RemoteMirror: Failed to set remote description."));
          } else {
            this._pc.addIceCandidate(new this._window.mozRTCIceCandidate(parsedData))
          }
        } else {
          log("RemoteMirror: data is null");
        }
      });
    } else if (response.type == "error") {
      log("RemoteMirror: Got socket error.");
      this._stopMirror();
    } else {
      log("RemoteMirror: Got unhandled socket event: " + response.type);
    }
  },

  _onSocketError: function(err) {
    log("RemoteMirror: Error socket.onerror: " + (err.data ? err.data : "NO DATA"));
    this._stopMirror();
  },

  _onSocketOpen: function() {
    this._open = true;
    if (this._jsonOffer) {
      let jsonOffer = this._jsonOffer + JSON_MESSAGE_TERMINATOR;
      this._socket.send(jsonOffer, jsonOffer.length);
      this._jsonOffer = null;
      this._sendIceCandidates();
    }
  },

  _sendIceCandidates: function() {
    if (this._socket && this._open) {
      this._iceCandidates.forEach(value => {
        value = value + JSON_MESSAGE_TERMINATOR;
        this._socket.send(value, value.length);
      });
      this._iceCandidates = [];
    }
  }
};
