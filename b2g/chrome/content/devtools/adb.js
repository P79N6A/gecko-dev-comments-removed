





"use strict";



const DEBUG = false;
var debug = function(str) {
  dump("AdbController: " + str + "\n");
}

let AdbController = {
  locked: undefined,
  remoteDebuggerEnabled: undefined,
  lockEnabled: undefined,
  disableAdbTimer: null,
  disableAdbTimeoutHours: 12,
  umsActive: false,

  setLockscreenEnabled: function(value) {
    this.lockEnabled = value;
    DEBUG && debug("setLockscreenEnabled = " + this.lockEnabled);
    this.updateState();
  },

  setLockscreenState: function(value) {
    this.locked = value;
    DEBUG && debug("setLockscreenState = " + this.locked);
    this.updateState();
  },

  setRemoteDebuggerState: function(value) {
    this.remoteDebuggerEnabled = value;
    DEBUG && debug("setRemoteDebuggerState = " + this.remoteDebuggerEnabled);
    this.updateState();
  },

  startDisableAdbTimer: function() {
    if (this.disableAdbTimer) {
      this.disableAdbTimer.cancel();
    } else {
      this.disableAdbTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      try {
        this.disableAdbTimeoutHours =
          Services.prefs.getIntPref("b2g.adb.timeout-hours");
      } catch (e) {
        
        
      }
    }
    if (this.disableAdbTimeoutHours <= 0) {
      DEBUG && debug("Timer to disable ADB not started due to zero timeout");
      return;
    }

    DEBUG && debug("Starting timer to disable ADB in " +
                   this.disableAdbTimeoutHours + " hours");
    let timeoutMilliseconds = this.disableAdbTimeoutHours * 60 * 60 * 1000;
    this.disableAdbTimer.initWithCallback(this, timeoutMilliseconds,
                                          Ci.nsITimer.TYPE_ONE_SHOT);
  },

  stopDisableAdbTimer: function() {
    DEBUG && debug("Stopping timer to disable ADB");
    if (this.disableAdbTimer) {
      this.disableAdbTimer.cancel();
      this.disableAdbTimer = null;
    }
  },

  notify: function(aTimer) {
    if (aTimer == this.disableAdbTimer) {
      this.disableAdbTimer = null;
      
      
      
      debug("ADB timer expired - disabling ADB\n");
      navigator.mozSettings.createLock().set(
        {'debugger.remote-mode': 'disabled'});
    }
  },

  updateState: function() {
    this.umsActive = false;
    this.storages = navigator.getDeviceStorages('sdcard');
    this.updateStorageState(0);
  },

  updateStorageState: function(storageIndex) {
    if (storageIndex >= this.storages.length) {
      
      
      this.updateStateInternal();
      return;
    }
    let storage = this.storages[storageIndex];
    DEBUG && debug("Checking availability of storage: '" + storage.storageName);

    let req = storage.available();
    req.onsuccess = function(e) {
      DEBUG && debug("Storage: '" + storage.storageName + "' is '" + e.target.result);
      if (e.target.result == 'shared') {
        
        
        this.umsActive = true;
        this.updateStateInternal();
        return;
      }
      this.updateStorageState(storageIndex + 1);
    }.bind(this);
    req.onerror = function(e) {

      Cu.reportError("AdbController: error querying storage availability for '" +
                     this.storages[storageIndex].storageName + "' (ignoring)\n");
      this.updateStorageState(storageIndex + 1);
    }.bind(this);
  },

  updateStateInternal: function() {
    DEBUG && debug("updateStateInternal: called");

    if (this.remoteDebuggerEnabled === undefined ||
        this.lockEnabled === undefined ||
        this.locked === undefined) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      DEBUG && debug("updateState: Waiting for all vars to be initialized");
      return;
    }

    
    
    let isDebugging = USBRemoteDebugger.isDebugging;
    DEBUG && debug("isDebugging=" + isDebugging);

    
    
    
    let sysUsbConfig = libcutils.property_get("sys.usb.config").split(",");
    let usbFuncActive = this.umsActive || isDebugging;
    usbFuncActive |= (sysUsbConfig.indexOf("rndis") >= 0);
    usbFuncActive |= (sysUsbConfig.indexOf("mtp") >= 0);

    let enableAdb = this.remoteDebuggerEnabled &&
      (!(this.lockEnabled && this.locked) || usbFuncActive);

    let useDisableAdbTimer = true;
    try {
      if (Services.prefs.getBoolPref("marionette.defaultPrefs.enabled")) {
        
        
        
        
        enableAdb = true;
        useDisableAdbTimer = false;
      }
    } catch (e) {
      
      
    }
    DEBUG && debug("updateState: enableAdb = " + enableAdb +
                   " remoteDebuggerEnabled = " + this.remoteDebuggerEnabled +
                   " lockEnabled = " + this.lockEnabled +
                   " locked = " + this.locked +
                   " usbFuncActive = " + usbFuncActive);

    
    let currentConfig = libcutils.property_get("persist.sys.usb.config");
    let configFuncs = currentConfig.split(",");
    let adbIndex = configFuncs.indexOf("adb");

    if (enableAdb) {
      
      if (adbIndex < 0) {
        configFuncs.push("adb");
      }
    } else {
      
      if (adbIndex >= 0) {
        configFuncs.splice(adbIndex, 1);
      }
    }
    let newConfig = configFuncs.join(",");
    if (newConfig != currentConfig) {
      DEBUG && debug("updateState: currentConfig = " + currentConfig);
      DEBUG && debug("updateState:     newConfig = " + newConfig);
      try {
        libcutils.property_set("persist.sys.usb.config", newConfig);
      } catch(e) {
        Cu.reportError("Error configuring adb: " + e);
      }
    }
    if (useDisableAdbTimer) {
      if (enableAdb && !usbFuncActive) {
        this.startDisableAdbTimer();
      } else {
        this.stopDisableAdbTimer();
      }
    }
  }
};

SettingsListener.observe("lockscreen.locked", false,
                         AdbController.setLockscreenState.bind(AdbController));
SettingsListener.observe("lockscreen.enabled", false,
                         AdbController.setLockscreenEnabled.bind(AdbController));
