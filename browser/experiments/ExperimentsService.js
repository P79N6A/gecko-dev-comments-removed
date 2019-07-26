



"use strict";

const {interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/experiments/Experiments.jsm");
Cu.import("resource://gre/modules/osfile.jsm")

function ExperimentsService() {
}

ExperimentsService.prototype = {
  classID: Components.ID("{f7800463-3b97-47f9-9341-b7617e6d8d49}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITimerCallback, Ci.nsIObserver]),

  notify: function (timer) {
    if (OS.Constants.Path.profileDir === undefined) {
      throw Error("Update timer fired before profile was initialized?");
    }
    Experiments.instance().updateManifest();
  },

  observe: function (subject, topic, data) {
    switch (topic) {
      case "profile-after-change":
        Experiments.instance(); 
        break;
    }
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ExperimentsService]);
