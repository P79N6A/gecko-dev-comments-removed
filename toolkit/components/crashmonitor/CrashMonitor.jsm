
































this.EXPORTED_SYMBOLS = [ "CrashMonitor" ];

const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/AsyncShutdown.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");

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

  







  path: (Services.metro && Services.metro.immersive) ?
    OS.Path.join(OS.Constants.Path.profileDir, "metro", "sessionCheckpoints.json"):
    OS.Path.join(OS.Constants.Path.profileDir, "sessionCheckpoints.json"),

  




  loadPreviousCheckpoints: function () {
    let deferred = Promise.defer();
    CrashMonitorInternal.previousCheckpoints = deferred.promise;

    let file = FileUtils.File(CrashMonitorInternal.path);
    NetUtil.asyncFetch(file, function(inputStream, status) {
      if (!Components.isSuccessCode(status)) {
        if (status != Cr.NS_ERROR_FILE_NOT_FOUND) {
          Cu.reportError("Error while loading crash monitor data: " + status);
        }

        deferred.resolve(null);
        return;
      }

      let data = NetUtil.readInputStreamToString(inputStream,
        inputStream.available(), { charset: "UTF-8" });

      let notifications = null;
      try {
        notifications = JSON.parse(data);
      } catch (ex) {
        Cu.reportError("Error while parsing crash monitor data: " + ex);
        deferred.resolve(null);
      }

      try {
        deferred.resolve(Object.freeze(notifications));
      } catch (ex) {
        
        
        
        deferred.reject(ex);
      }
    });

    return deferred.promise;
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
    OS.File.makeDir(OS.Path.join(OS.Constants.Path.profileDir, "metro"));
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
