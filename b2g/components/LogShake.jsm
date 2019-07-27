






















'use strict';

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');

XPCOMUtils.defineLazyModuleGetter(this, 'LogCapture', 'resource://gre/modules/LogCapture.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'LogParser', 'resource://gre/modules/LogParser.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'OS', 'resource://gre/modules/osfile.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Promise', 'resource://gre/modules/Promise.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'Services', 'resource://gre/modules/Services.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'SystemAppProxy', 'resource://gre/modules/SystemAppProxy.jsm');

XPCOMUtils.defineLazyServiceGetter(this, 'powerManagerService',
                                   '@mozilla.org/power/powermanagerservice;1',
                                   'nsIPowerManagerService');

XPCOMUtils.defineLazyServiceGetter(this, 'volumeService',
                                   '@mozilla.org/telephony/volume-service;1',
                                   'nsIVolumeService');

this.EXPORTED_SYMBOLS = ['LogShake'];

function debug(msg) {
  dump('LogShake.jsm: '+msg+'\n');
}





const EXCITEMENT_THRESHOLD = 500;
const DEVICE_MOTION_EVENT = 'devicemotion';
const SCREEN_CHANGE_EVENT = 'screenchange';
const CAPTURE_LOGS_START_EVENT = 'capture-logs-start';
const CAPTURE_LOGS_ERROR_EVENT = 'capture-logs-error';
const CAPTURE_LOGS_SUCCESS_EVENT = 'capture-logs-success';

let LogShake = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),
  




  deviceMotionEnabled: false,

  



  captureRequested: false,

  


  LOGS_WITH_PARSERS: {
    '/dev/log/main': LogParser.prettyPrintLogArray,
    '/dev/log/system': LogParser.prettyPrintLogArray,
    '/dev/log/radio': LogParser.prettyPrintLogArray,
    '/dev/log/events': LogParser.prettyPrintLogArray,
    '/proc/cmdline': LogParser.prettyPrintArray,
    '/proc/kmsg': LogParser.prettyPrintArray,
    '/proc/meminfo': LogParser.prettyPrintArray,
    '/proc/uptime': LogParser.prettyPrintArray,
    '/proc/version': LogParser.prettyPrintArray,
    '/proc/vmallocinfo': LogParser.prettyPrintArray,
    '/proc/vmstat': LogParser.prettyPrintArray
  },

  


  init: function() {
    
    
    
    

    
    
    this.handleScreenChangeEvent({ detail: {
      screenEnabled: true
    }});

    SystemAppProxy.addEventListener(SCREEN_CHANGE_EVENT, this, false);

    Services.obs.addObserver(this, 'xpcom-shutdown', false);
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
    }
  },

  


  observe: function(subject, topic) {
    if (topic === 'xpcom-shutdown') {
      this.uninit();
    }
  },

  startDeviceMotionListener: function() {
    if (!this.deviceMotionEnabled) {
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
      if (!this.captureRequested) {
        this.captureRequested = true;
        SystemAppProxy._sendCustomEvent(CAPTURE_LOGS_START_EVENT, {});
        this.captureLogs().then(logResults => {
          
          SystemAppProxy._sendCustomEvent(CAPTURE_LOGS_SUCCESS_EVENT, {
            logFilenames: logResults.logFilenames,
            logPrefix: logResults.logPrefix
          });
          this.captureRequested = false;
        },
        error => {
          
          SystemAppProxy._sendCustomEvent(CAPTURE_LOGS_ERROR_EVENT, {error: error});
          this.captureRequested = false;
        });
      }
    }
  },

  handleScreenChangeEvent: function(event) {
    if (event.detail.screenEnabled) {
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

    for (let loc in this.LOGS_WITH_PARSERS) {
      let logArray;
      try {
        logArray = LogCapture.readLogFile(loc);
        if (!logArray) {
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
    Services.obs.removeObserver(this, 'xpcom-shutdown');
  }
};

function getLogFilename(logLocation) {
  
  let logName = logLocation.replace(/\//g, '-');
  if (logName[0] === '-') {
    logName = logName.substring(1);
  }
  return logName + '.log';
}

function getSdcardPrefix() {
  return volumeService.getVolumeByName('sdcard').mountPoint;
}

function getLogDirectory() {
  let d = new Date();
  d = new Date(d.getTime() - d.getTimezoneOffset() * 60000);
  let timestamp = d.toISOString().slice(0, -5).replace(/[:T]/g, '-');
  
  return OS.Path.join('logs', timestamp);
}




function saveLogs(logArrays) {
  if (!logArrays || Object.keys(logArrays).length === 0) {
    return Promise.resolve({
      logFilenames: [],
      logPrefix: ''
    });
  }

  let sdcardPrefix, dirName;
  try {
    sdcardPrefix = getSdcardPrefix();
    dirName = getLogDirectory();
  } catch(e) {
    
    
    return Promise.reject(e);
  }

  debug('making a directory all the way from '+sdcardPrefix+' to '+(sdcardPrefix + '/' + dirName));
  return OS.File.makeDir(OS.Path.join(sdcardPrefix, dirName), {from: sdcardPrefix})
    .then(function() {
    
    let logFilenames = [];
    let saveRequests = [];

    for (let logLocation in logArrays) {
      debug('requesting save of ' + logLocation);
      let logArray = logArrays[logLocation];
      
      
      
      let filename = OS.Path.join(dirName, getLogFilename(logLocation));
      logFilenames.push(filename);
      let saveRequest = OS.File.writeAtomic(OS.Path.join(sdcardPrefix, filename), logArray);
      saveRequests.push(saveRequest);
    }

    return Promise.all(saveRequests).then(function() {
      debug('returning logfilenames: '+logFilenames.toSource());
      return {
        logFilenames: logFilenames,
        logPrefix: dirName
      };
    });
  });
}

LogShake.init();
this.LogShake = LogShake;
