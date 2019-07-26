





const {Cc, Ci, Cu, Cr} = require("chrome");

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/devtools/event-emitter.js");

exports.PREF_ORIG_SOURCES = "devtools.styleeditor.source-maps-enabled";





function PrefObserver(branchName) {
  this.branchName = branchName;
  this.branch = Services.prefs.getBranch(branchName);
  this.branch.addObserver("", this, false);

  EventEmitter.decorate(this);
}

exports.PrefObserver = PrefObserver;

PrefObserver.prototype = {
  observe: function(subject, topic, data) {
    if (topic == "nsPref:changed") {
      this.emit(this.branchName + data);
    }
  },

  destroy: function() {
    if (this.branch) {
      this.branch.removeObserver('', this);
    }
  }
};