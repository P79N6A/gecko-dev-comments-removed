
































this.EXPORTED_SYMBOLS = [ "CrashMonitor" ];

const Cu = Components.utils;

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
    let promise = Task.spawn(function () {
      let notifications;
      try {
        let decoder = new TextDecoder();
        let data = yield OS.File.read(CrashMonitorInternal.path);
        let contents = decoder.decode(data);
        notifications = JSON.parse(contents);
      } catch (ex if ex instanceof OS.File.Error && ex.becauseNoSuchFile) {
        
        throw new Task.Result(null);
      } catch (ex) {
        Cu.reportError("Error while loading crash monitor data: " + ex);
        throw new Task.Result(null);
      }
      throw new Task.Result(Object.freeze(notifications));
    });

    CrashMonitorInternal.previousCheckpoints = promise;
    return promise;
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

    
    AsyncShutdown.profileBeforeChange.addBlocker(
      "CrashMonitor: Writing notifications to file after receiving profile-before-change",
      CrashMonitorInternal.profileBeforeChangeDeferred.promise
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
