




"use strict";

this.EXPORTED_SYMBOLS = ["MediaPlayerApp"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");
let log = Cu.import("resource://gre/modules/AndroidLog.jsm", {}).AndroidLog.d.bind(null, "MediaPlayerApp");


function send(type, data, callback) {
  let msg = {
    type: type
  };

  for (let i in data) {
    msg[i] = data[i];
  }

  sendMessageToJava(msg, callback);
}



function MediaPlayerApp(service) {
  this.service = service;
  this.location = service.location;
  this.id = service.uuid;
}

MediaPlayerApp.prototype = {
  start: function start(callback) {
    send("MediaPlayer:Start", { id: this.id }, (result) => {
      if (callback) callback(true);
    });
  },

  stop: function stop(callback) {
    send("MediaPlayer:Stop", { id: this.id }, (result) => {
      if (callback) callback(true);
    });
  },

  remoteMedia: function remoteMedia(callback, listener) {
    if (callback) {
      callback(new RemoteMedia(this.id, listener));
    }
  },

  mirror: function mirror(callback) {
    send("MediaPlayer:Mirror", { id: this.id }, (result) => {
      if (callback) callback(true);
    });
  }
}



function RemoteMedia(id, listener) {
  this._id = id;
  this._listener = listener;

  if ("onRemoteMediaStart" in this._listener) {
    Services.tm.mainThread.dispatch((function() {
      this._listener.onRemoteMediaStart(this);
    }).bind(this), Ci.nsIThread.DISPATCH_NORMAL);
  }
}

RemoteMedia.prototype = {
  shutdown: function shutdown() {
    this._send("MediaPlayer:End", {}, (result) => {
      this._status = "shutdown";
      if ("onRemoteMediaStop" in this._listener) {
        this._listener.onRemoteMediaStop(this);
      }
    });
  },

  play: function play() {
    this._send("MediaPlayer:Play", {}, (result) => {
      this._status = "started";
    });
  },

  pause: function pause() {
    this._send("MediaPlayer:Pause", {}, (result) => {
      this._status = "paused";
    });
  },

  load: function load(aData) {
    this._send("MediaPlayer:Load", aData, (result) => {
      this._status = "started";
    })
  },

  get status() {
    return this._status;
  },

  _send: function(msg, data, callback) {
    data.id = this._id;
    send(msg, data, callback);
  }
}
