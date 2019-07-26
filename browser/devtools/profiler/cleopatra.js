



"use strict";

let { Cu }       = require("chrome");
let { defer }    = Cu.import("resource://gre/modules/Promise.jsm", {}).Promise;
let EventEmitter = require("devtools/toolkit/event-emitter");

const { PROFILE_IDLE, PROFILE_COMPLETED, PROFILE_RUNNING } = require("devtools/profiler/consts");















function Cleopatra(panel, opts) {
  let doc = panel.document;
  let win = panel.window;
  let { uid, name } = opts;
  let spd = opts.showPlatformData;
  let ext = opts.external;

  EventEmitter.decorate(this);

  this.isReady = false;
  this.isStarted = false;
  this.isFinished = false;

  this.panel = panel;
  this.uid = uid;
  this.name = name;

  this.iframe = doc.createElement("iframe");
  this.iframe.setAttribute("flex", "1");
  this.iframe.setAttribute("id", "profiler-cleo-" + uid);
  this.iframe.setAttribute("src", "cleopatra.html?uid=" + uid + "&spd=" + spd + "&ext=" + ext);
  this.iframe.setAttribute("hidden", "true");

  
  
  

  doc.getElementById("profiler-report").appendChild(this.iframe);
  win.addEventListener("message", (event) => {
    if (parseInt(event.data.uid, 10) !== parseInt(this.uid, 10)) {
      return;
    }

    switch (event.data.status) {
      case "loaded":
        this.isReady = true;
        this.emit("ready");
        break;
      case "displaysource":
        this.panel.displaySource(event.data.data);
    }
  });
}

Cleopatra.prototype = {
  



  get contentWindow() {
    if (!this.iframe) {
      return null;
    }

    try {
      return this.iframe.contentWindow;
    } catch (err) {
      return null;
    }
  },

  show: function () {
    this.iframe.removeAttribute("hidden");
  },

  hide: function () {
    this.iframe.setAttribute("hidden", true);
  },

  









  parse: function (data, onParsed) {
    if (!this.isReady) {
      return void this.on("ready", this.parse.bind(this, data, onParsed));
    }

    this.message({ task: "receiveProfileData", rawProfile: data }).then(() => {
      let poll = () => {
        let wait = this.panel.window.setTimeout.bind(null, poll, 100);
        let trail = this.contentWindow.gBreadcrumbTrail;

        if (!trail) {
          return wait();
        }

        if (!trail._breadcrumbs || !trail._breadcrumbs.length) {
          return wait();
        }

        onParsed();
      };

      poll();
    });
  },

  






  message: function (data) {
    let deferred = defer();
    data = JSON.stringify(data);

    let send = () => {
      if (!this.contentWindow)
        setTimeout(send, 50);

      this.contentWindow.postMessage(data, "*");
      deferred.resolve();
    };

    send();
    return deferred.promise;
  },

  


  destroy: function () {
    this.isReady = null;
    this.panel = null;
    this.uid = null;
    this.iframe = null;
    this.messages = null;
  }
};

module.exports = Cleopatra;

