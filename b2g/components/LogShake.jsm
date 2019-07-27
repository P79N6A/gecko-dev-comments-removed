























"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");


const PR_RDWR        = 0x04;
const PR_CREATE_FILE = 0x08;
const PR_TRUNCATE    = 0x20;

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





const EXCITEMENT_FILTER_ALPHA = 0.2;
const DEVICE_MOTION_EVENT = "devicemotion";
const SCREEN_CHANGE_EVENT = "screenchange";
const CAPTURE_LOGS_CONTENT_EVENT = "requestSystemLogs";
const CAPTURE_LOGS_START_EVENT = "capture-logs-start";
const CAPTURE_LOGS_ERROR_EVENT = "capture-logs-error";
const CAPTURE_LOGS_SUCCESS_EVENT = "capture-logs-success";

let LogShake = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  


  qaModeEnabled: false,

  




  deviceMotionEnabled: false,

  



  screenEnabled: true,

  



  listenToDeviceMotion: true,

  



  captureRequested: false,

  


  excitement: 0,

  


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

    
    this.excitement = 0;

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

  enableQAMode: function() {
    debug("Enabling QA Mode");
    this.qaModeEnabled = true;
  },

  disableQAMode: function() {
    debug("Disabling QA Mode");
    this.qaModeEnabled = false;
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

    let acc = event.accelerationIncludingGravity;

    
    
    let newExcitement = acc.x * acc.x + acc.y * acc.y + acc.z * acc.z;
    this.excitement += (newExcitement - this.excitement) * EXCITEMENT_FILTER_ALPHA;

    if (this.excitement > EXCITEMENT_THRESHOLD) {
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
        logPaths: logResults.logPaths,
        logFilenames: logResults.logFilenames
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
    return this.saveLogs(logArrays);
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
        logArrays["screenshot.png"] = screenshot;
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

  


  saveLogs: function(logArrays) {
    if (!logArrays || Object.keys(logArrays).length === 0) {
      return Promise.reject("Zero logs saved");
    }

    if (this.qaModeEnabled) {
      return makeBaseLogsDirectory().then(writeLogArchive(logArrays),
                                          rejectFunction("Error making base log directory"));
    } else {
      return makeBaseLogsDirectory().then(makeLogsDirectory,
                                          rejectFunction("Error making base log directory"))
                                    .then(writeLogFiles(logArrays),
                                          rejectFunction("Error creating log directory"));
    }
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

function getLogIdentifier() {
  let d = new Date();
  d = new Date(d.getTime() - d.getTimezoneOffset() * 60000);
  let timestamp = d.toISOString().slice(0, -5).replace(/[:T]/g, "-");
  return timestamp;
}

function rejectFunction(message) {
  return function(err) {
    debug(message + ": " + err);
    return Promise.reject(err);
  };
}

function makeBaseLogsDirectory() {
  let sdcardPrefix;
  try {
    sdcardPrefix = getSdcardPrefix();
  } catch(e) {
    
    return Promise.reject(e);
  }

  let dirNameRoot = getLogDirectoryRoot();

  let logsRoot = OS.Path.join(sdcardPrefix, dirNameRoot);

  debug("Creating base log directory at root " + sdcardPrefix);

  return OS.File.makeDir(logsRoot, {from: sdcardPrefix}).then(
    function() {
      return {
        sdcardPrefix: sdcardPrefix,
        basePrefix: dirNameRoot
      };
    }
  );
}

function makeLogsDirectory({sdcardPrefix, basePrefix}) {
  let dirName = getLogIdentifier();

  let logsRoot = OS.Path.join(sdcardPrefix, basePrefix);
  let logsDir = OS.Path.join(logsRoot, dirName);

  debug("Creating base log directory at root " + sdcardPrefix);
  debug("Final created directory will be " + logsDir);

  return OS.File.makeDir(logsDir, {ignoreExisting: false}).then(
    function() {
      debug("Created: " + logsDir);
      return {
        logPrefix: OS.Path.join(basePrefix, dirName),
        sdcardPrefix: sdcardPrefix
      };
    },
    rejectFunction("Error at OS.File.makeDir for " + logsDir)
  );
}

function getFile(filename) {
  let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
  file.initWithPath(filename);
  return file;
}







function makeZipFile(absoluteZipFilename, logArrays) {
  let logFilenames = [];
  let zipWriter = Cc["@mozilla.org/zipwriter;1"].createInstance(Ci.nsIZipWriter);
  let zipFile = getFile(absoluteZipFilename);
  zipWriter.open(zipFile, PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE);

  for (let logLocation in logArrays) {
    let logArray = logArrays[logLocation];
    let logFilename = getLogFilename(logLocation);
    logFilenames.push(logFilename);

    debug("Adding " + logFilename + " to the zip");
    let logArrayStream = Cc["@mozilla.org/io/arraybuffer-input-stream;1"]
                           .createInstance(Ci.nsIArrayBufferInputStream);
    
    
    logArrayStream.setData(logArray.buffer, logArray.byteOffset || 0,
                           logArray.byteLength);

    zipWriter.addEntryStream(logFilename, Date.now(),
                             Ci.nsIZipWriter.COMPRESSION_DEFAULT,
                             logArrayStream, false);
  }
  zipWriter.close();

  return logFilenames;
}

function writeLogArchive(logArrays) {
  return function({sdcardPrefix, basePrefix}) {
    
    

    let zipFilename = getLogIdentifier() + "-logs.zip";
    let zipPath = OS.Path.join(basePrefix, zipFilename);
    let zipPrefix = OS.Path.dirname(zipPath);
    let absoluteZipPath = OS.Path.join(sdcardPrefix, zipPath);

    debug("Creating zip file at " + zipPath);
    let logFilenames = [];
    try {
      logFilenames = makeZipFile(absoluteZipPath, logArrays);
    } catch(e) {
      return Promise.reject(e);
    }
    debug("Zip file created");

    return {
      logFilenames: logFilenames,
      logPaths: [zipPath],
      compressed: true
    };
  };
}

function writeLogFiles(logArrays) {
  return function({sdcardPrefix, logPrefix}) {
    
    let logFilenames = [];
    let logPaths = [];
    let saveRequests = [];

    for (let logLocation in logArrays) {
      debug("Requesting save of " + logLocation);
      let logArray = logArrays[logLocation];
      let logFilename = getLogFilename(logLocation);
      
      
      
      let localPath = OS.Path.join(logPrefix, logFilename);

      logFilenames.push(logFilename);
      logPaths.push(localPath);

      let absolutePath = OS.Path.join(sdcardPrefix, localPath);
      let saveRequest = OS.File.writeAtomic(absolutePath, logArray);
      saveRequests.push(saveRequest);
    }

    return Promise.all(saveRequests).then(
      function() {
        return {
          logFilenames: logFilenames,
          logPaths: logPaths,
          compressed: false
        };
      },
      rejectFunction("Error at some save request")
    );
  };
}

LogShake.init();
this.LogShake = LogShake;
