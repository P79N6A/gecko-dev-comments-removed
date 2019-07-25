




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function TestPilotComponent() {}
TestPilotComponent.prototype = {
  classDescription: "Test Pilot Component",
  contractID: "@mozilla.org/testpilot/service;1",
  classID: Components.ID("{e6e5e58f-7977-485a-b076-2f74bee2677b}"),
  _xpcom_categories: [{ category: "profile-after-change" }],
  _startupTimer: null,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  observe: function TPC__observe(subject, topic, data) {
    let os = Cc["@mozilla.org/observer-service;1"].
        getService(Ci.nsIObserverService);
    switch (topic) {
    case "profile-after-change":
      os.addObserver(this, "sessionstore-windows-restored", true);
      break;
    case "sessionstore-windows-restored":
      

      os.removeObserver(this, "sessionstore-windows-restored", false);
      



      this._startupTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      this._startupTimer.initWithCallback(
        {notify: function(timer) {
           Cu.import("resource://testpilot/modules/setup.js");
           TestPilotSetup.globalStartup();
         }}, 10000, Ci.nsITimer.TYPE_ONE_SHOT);
      break;
    }
  }
};

const components = [TestPilotComponent];
var NSGetFactory, NSGetModule;
if (XPCOMUtils.generateNSGetFactory)
  NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
else
  NSGetModule = XPCOMUtils.generateNSGetModule(components);
