






















"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LogCapture", "resource://gre/modules/LogCapture.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "LogParser", "resource://gre/modules/LogParser.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS", "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise", "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services", "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SystemAppProxy", "resource://gre/modules/SystemAppProxy.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "powerManagerService",
                                   "@mozilla.org/power/powermanagerservice;1",
                                   "nsIPowerManagerService");

XPCOMUtils.defineLazyServiceGetter(this, "volumeService",
                                   "@mozilla.org/telephony/volume-service;1",
                                   "nsIVolumeService");

this.EXPORTED_SYMBOLS = ["LogShake"];

function debug(msg) {
  dump("LogShake.jsm: "+msg+"\n");
}





const EXCITEMENT_THRESHOLD = 500;
const DEVICE_MOTION_EVENT = "devicemotion";
const SCREEN_CHANGE_EVENT = "screenchange";
const CAPTURE_LOGS_CONTENT_EVENT = "requestSystemLogs";
const CAPTURE_LOGS_START_EVENT = "capture-logs-start";
const CAPTURE_LOGS_ERROR_EVENT = "capture-logs-error";
const CAPTURE_LOGS_SUCCESS_EVENT = "capture-logs-success";

let LogShake = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),
  




  deviceMotionEnabled: false,

  



  screenEnabled: true,

  



  listenToDeviceMotion: true,

  



  captureRequested: false,

  


  LOGS_WITH_PARSERS: {
    "/dev/log/main": LogParser.prettyPrintLogArray,
    "/dev/log/system": LogParser.prettyPrintLogArray,
    "/dev/log/radio": LogParser.prettyPrintLogArray,
    "/dev/log/events": LogParser.prettyPrintLogArray,
    "/proc/cmdline": LogParser.prettyPrintArray,
    "/proc/kmsg": LogParser.prettyPrintArray,
    "/proc/last_kmsg": LogParser.prettyPrintArray,
    "/proc/meminfo": LogParser.prettyPrintArray,
    "/proc/uptime": LogParser.prettyPrintArray,
    "/proc/version": LogParser.prettyPrintArray,
    "/proc/vmallocinfo": LogParser.prettyPrintArray,
    "/proc/vmstat": LogParser.prettyPrintArray,
    "/system/b2g/application.ini": LogParser.prettyPrintArray
  },

  


  init: function() {
    
    
    
    

    
    
    this.handleScreenChangeEvent({ detail: {
      screenEnabled: true
    }});

    SystemAppProxy.addEventListener(CAPTURE_LOGS_CONTENT_EVENT, this, false);
    SystemAppProxy.addEventListener(SCREEN_CHANGE_EVENT, this, false);

    Services.obs.addObserver(this, "xpcom-shutdown", false);
  },

  


  handleEvent: function(event) {
    switch (event.type) {
    case DEVICE_MOTION_EVENT:
      if (!this.deviceMotionEnabled) {
        return;
      }
      this.handleDeviceMotionEvent(event);
      break;

    case SCREEN_CHANGE_EVENT:
      this.handleScreenChangeEvent(event);
      break;

    case CAPTURE_LOGS_CONTENT_EVENT:
      this.startCapture();
      break;
    }
  },

  


  observe: function(subject, topic) {
    if (topic === "xpcom-shutdown") {
      this.uninit();
    }
  },

  enableDeviceMotionListener: function() {
    this.listenToDeviceMotion = true;
    this.startDeviceMotionListener();
  },

  disableDeviceMotionListener: function() {
    this.listenToDeviceMotion = false;
    this.stopDeviceMotionListener();
  },

  startDeviceMotionListener: function() {
    if (!this.deviceMotionEnabled &&
        this.listenToDeviceMotion &&
        this.screenEnabled) {
      SystemAppProxy.addEventListener(DEVICE_MOTION_EVENT, this, false);
      this.deviceMotionEnabled = true;
    }
  },

  stopDeviceMotionListener: function() {
    SystemAppProxy.removeEventListener(DEVICE_MOTION_EVENT, this, false);
    this.deviceMotionEnabled = false;
  },

  



  handleDeviceMotionEvent: function(event) {
    
    
    if (!this.deviceMotionEnabled) {
      return;
    }

    var acc = event.accelerationIncludingGravity;

    var excitement = acc.x * acc.x + acc.y * acc.y + acc.z * acc.z;

    if (excitement > EXCITEMENT_THRESHOLD) {
      this.startCapture();
    }
  },

  startCapture: function() {
    if (this.captureRequested) {
      return;
    }
    this.captureRequested = true;
    SystemAppProxy._sendCustomEvent(CAPTURE_LOGS_START_EVENT, {});
    this.captureLogs().then(logResults => {
      
      SystemAppProxy._sendCustomEvent(CAPTURE_LOGS_SUCCESS_EVENT, {
        logFilenames: logResults.logFilenames,
        logPrefix: logResults.logPrefix
      });
      this.captureRequested = false;
    }, error => {
      
      SystemAppProxy._sendCustomEvent(CAPTURE_LOGS_ERROR_EVENT, {error: error});
      this.captureRequested = false;
    });
  },

  handleScreenChangeEvent: function(event) {
    this.screenEnabled = event.detail.screenEnabled;
    if (this.screenEnabled) {
      this.startDeviceMotionListener();
    } else {
      this.stopDeviceMotionListener();
    }
  },

  



  captureLogs: function() {
    let logArrays = this.readLogs();
    return saveLogs(logArrays);
  },

  


  readLogs: function() {
    let logArrays = {};

    try {
      logArrays["properties"] =
        LogParser.prettyPrintPropertiesArray(LogCapture.readProperties());
    } catch (ex) {
      Cu.reportError("Unable to get device properties: " + ex);
    }

    
    try {
      LogCapture.readAboutMemory().then(aboutMemory => {
        let file = OS.Path.basename(aboutMemory);
        let logArray;
        try {
          logArray = LogCapture.readLogFile(aboutMemory);
          if (!logArray) {
            debug("LogCapture.readLogFile() returned nothing about:memory ");
          }
          
          OS.File.remove(aboutMemory);
        } catch (ex) {
          Cu.reportError("Unable to handle about:memory dump: " + ex);
        }
        logArrays[file] = LogParser.prettyPrintArray(logArray);
      });
    } catch (ex) {
      Cu.reportError("Unable to get about:memory dump: " + ex);
    }

    try {
      LogCapture.getScreenshot().then(screenshot => {
        logArrays["logshake-screenshot.png"] = screenshot;
      });
    } catch (ex) {
      Cu.reportError("Unable to get screenshot dump: " + ex);
    }

    for (let loc in this.LOGS_WITH_PARSERS) {
      let logArray;
      try {
        logArray = LogCapture.readLogFile(loc);
        if (!logArray) {
          debug("LogCapture.readLogFile() returned nothing for: " + loc);
          continue;
        }
      } catch (ex) {
        Cu.reportError("Unable to LogCapture.readLogFile('" + loc + "'): " + ex);
        continue;
      }

      try {
        logArrays[loc] = this.LOGS_WITH_PARSERS[loc](logArray);
      } catch (ex) {
        Cu.reportError("Unable to parse content of '" + loc + "': " + ex);
        continue;
      }
    }
    return logArrays;
  },

  


  uninit: function() {
    this.stopDeviceMotionListener();
    SystemAppProxy.removeEventListener(SCREEN_CHANGE_EVENT, this, false);
    Services.obs.removeObserver(this, "xpcom-shutdown");
  }
};

function getLogFilename(logLocation) {
  
  let logName = logLocation.replace(/\//g, "-");
  if (logName[0] === "-") {
    logName = logName.substring(1);
  }

  
  let extension = ".log";
  let logLocationExt = logLocation.split(".");
  if (logLocationExt.length > 1) {
    
    extension = "";
  }

  return logName + extension;
}

function getSdcardPrefix() {
  return volumeService.getVolumeByName("sdcard").mountPoint;
}

function getLogDirectoryRoot() {
  return "logs";
}

function getLogDirectory() {
  let d = new Date();
  d = new Date(d.getTime() - d.getTimezoneOffset() * 60000);
  let timestamp = d.toISOString().slice(0, -5).replace(/[:T]/g, "-");
  return timestamp;
}




function saveLogs(logArrays) {
  if (!logArrays || Object.keys(logArrays).length === 0) {
    return Promise.resolve({
      logFilenames: [],
      logPrefix: ""
    });
  }

  let sdcardPrefix, dirNameRoot, dirName;
  try {
    sdcardPrefix = getSdcardPrefix();
    dirNameRoot = getLogDirectoryRoot();
    dirName = getLogDirectory();
  } catch(e) {
    
    
    return Promise.reject(e);
  }

  debug("making a directory all the way from " + sdcardPrefix + " to " + (sdcardPrefix + "/" + dirNameRoot + "/" + dirName) );
  let logsRoot = OS.Path.join(sdcardPrefix, dirNameRoot);
  return OS.File.makeDir(logsRoot, {from: sdcardPrefix}).then(
    function() {
      debug("First OS.File.makeDir done");
      let logsDir = OS.Path.join(logsRoot, dirName);
      debug("Creating " + logsDir);
      return OS.File.makeDir(logsDir, {ignoreExisting: false}).then(
        function() {
          debug("Created: " + logsDir);
          
          let logFilenames = [];
          let saveRequests = [];

          debug("Will now traverse logArrays: " + logArrays.length);

          for (let logLocation in logArrays) {
            debug("requesting save of " + logLocation);
            let logArray = logArrays[logLocation];
            
            
            
            let filename = OS.Path.join(dirNameRoot, dirName, getLogFilename(logLocation));
            logFilenames.push(filename);
            let saveRequest = OS.File.writeAtomic(OS.Path.join(sdcardPrefix, filename), logArray);
            saveRequests.push(saveRequest);
          }

          return Promise.all(saveRequests).then(
            function() {
              debug("returning logfilenames: "+logFilenames.toSource());
              return {
                logFilenames: logFilenames,
                logPrefix: OS.Path.join(dirNameRoot, dirName)
              };
            }, function(err) {
              debug("Error at some save request: " + err);
              return Promise.reject(err);
            });
        }, function(err) {
          debug("Error at OS.File.makeDir for " + logsDir + ": " + err);
          return Promise.reject(err);
        });
    }, function(err) {
      debug("Error at first OS.File.makeDir: " + err);
      return Promise.reject(err);
    });
}

LogShake.init();
this.LogShake = LogShake;
