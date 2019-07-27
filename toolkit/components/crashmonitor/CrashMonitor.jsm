
































this.EXPORTED_SYMBOLS = [ "CrashMonitor" ];

const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/AsyncShutdown.jsm");

const NOTIFICATIONS = [
  "final-ui-startup",
  "sessionstore-windows-restored",
  "quit-application-granted",
  "quit-application",
  "profile-change-net-teardown",
  "profile-change-teardown",
  "profile-before-change",
  "sessionstore-final-state-write-complete"
];

let CrashMonitorInternal = {

  








  checkpoints: {},

  








  previousCheckpoints: null,

  
  profileBeforeChangeDeferred: Promise.defer(),

  





  path: OS.Path.join(OS.Constants.Path.profileDir, "sessionCheckpoints.json"),

  




  loadPreviousCheckpoints: function () {
    this.previousCheckpoints = Task.spawn(function*() {
      let data;
      try {
        data = yield OS.File.read(CrashMonitorInternal.path, { encoding: "utf-8" });
      } catch (ex if ex instanceof OS.File.Error) {
        if (!ex.becauseNoSuchFile) {
          Cu.reportError("Error while loading crash monitor data: " + ex.toString());
        }

        return null;
      }

      let notifications;
      try {
        notifications = JSON.parse(data);
      } catch (ex) {
        Cu.reportError("Error while parsing crash monitor data: " + ex);
        return null;
      }

      
      if (Object(notifications) !== notifications) {
        Cu.reportError("Error while parsing crash monitor data: invalid monitor data");
        return null;
      }

      return Object.freeze(notifications);
    });

    return this.previousCheckpoints;
  }
};

this.CrashMonitor = {

  







  get previousCheckpoints() {
    if (!CrashMonitorInternal.initialized) {
      throw new Error("CrashMonitor must be initialized before getting previous checkpoints");
    }

    return CrashMonitorInternal.previousCheckpoints
  },

  






  init: function () {
    if (CrashMonitorInternal.initialized) {
      throw new Error("CrashMonitor.init() must only be called once!");
    }

    let promise = CrashMonitorInternal.loadPreviousCheckpoints();
    
    
    CrashMonitorInternal.checkpoints["profile-after-change"] = true;

    NOTIFICATIONS.forEach(function (aTopic) {
      Services.obs.addObserver(this, aTopic, false);
    }, this);

    
    OS.File.profileBeforeChange.addBlocker(
      "CrashMonitor: Writing notifications to file after receiving profile-before-change",
      CrashMonitorInternal.profileBeforeChangeDeferred.promise,
      () => this.checkpoints
    );

    CrashMonitorInternal.initialized = true;
    return promise;
  },

  




  observe: function (aSubject, aTopic, aData) {
    if (!(aTopic in CrashMonitorInternal.checkpoints)) {
      
      
      CrashMonitorInternal.checkpoints[aTopic] = true;
      Task.spawn(function() {
        try {
          let data = JSON.stringify(CrashMonitorInternal.checkpoints);

          





          yield OS.File.writeAtomic(
            CrashMonitorInternal.path,
            data, {tmpPath: CrashMonitorInternal.path + ".tmp"});

        } finally {
          
          if (aTopic == "profile-before-change") {
            CrashMonitorInternal.profileBeforeChangeDeferred.resolve();
          }
        }
      });
    }

    if (NOTIFICATIONS.every(elem => elem in CrashMonitorInternal.checkpoints)) {
      
      NOTIFICATIONS.forEach(function (aTopic) {
        Services.obs.removeObserver(this, aTopic);
      }, this);
    }
  }
};
Object.freeze(this.CrashMonitor);
