





"use strict;"

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');

#ifdef MOZ_WIDGET_GONK
XPCOMUtils.defineLazyGetter(this, "libcutils", function () {
  Cu.import("resource://gre/modules/systemlibs.js");
  return libcutils;
});
#endif

XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");



var SettingsListener = {
  _callbacks: {},

  init: function sl_init() {
    if ('mozSettings' in navigator && navigator.mozSettings) {
      navigator.mozSettings.onsettingchange = this.onchange.bind(this);
    }
  },

  onchange: function sl_onchange(evt) {
    var callback = this._callbacks[evt.settingName];
    if (callback) {
      callback(evt.settingValue);
    }
  },

  observe: function sl_observe(name, defaultValue, callback) {
    var settings = window.navigator.mozSettings;
    if (!settings) {
      window.setTimeout(function() { callback(defaultValue); });
      return;
    }

    if (!callback || typeof callback !== 'function') {
      throw new Error('Callback is not a function');
    }

    var req = settings.createLock().get(name);
    req.addEventListener('success', (function onsuccess() {
      callback(typeof(req.result[name]) != 'undefined' ?
        req.result[name] : defaultValue);
    }));

    this._callbacks[name] = callback;
  }
};

SettingsListener.init();



SettingsListener.observe('debug.console.enabled', true, function(value) {
  Services.prefs.setBoolPref('consoleservice.enabled', value);
  Services.prefs.setBoolPref('layout.css.report_errors', value);
});


SettingsListener.observe('language.current', 'en-US', function(value) {
  Services.prefs.setCharPref('general.useragent.locale', value);

  let prefName = 'intl.accept_languages';
  if (Services.prefs.prefHasUserValue(prefName)) {
    Services.prefs.clearUserPref(prefName);
  }

  let intl = '';
  try {
    intl = Services.prefs.getComplexValue(prefName,
                                          Ci.nsIPrefLocalizedString).data;
  } catch(e) {}

  
  
  
  
  
  if (!((new RegExp('^' + value + '[^a-z-_] *[,;]?', 'i')).test(intl))) {
    value = value + ', ' + intl;
  } else {
    value = intl;
  }
  Services.prefs.setCharPref(prefName, value);

  if (shell.hasStarted() == false) {
    shell.start();
  }
});


(function RILSettingsToPrefs() {
  let strPrefs = ['ril.mms.mmsc', 'ril.mms.mmsproxy'];
  strPrefs.forEach(function(key) {
    SettingsListener.observe(key, "", function(value) {
      Services.prefs.setCharPref(key, value);
    });
  });

  ['ril.mms.mmsport'].forEach(function(key) {
    SettingsListener.observe(key, null, function(value) {
      if (value != null) {
        Services.prefs.setIntPref(key, value);
      }
    });
  });

  SettingsListener.observe('ril.mms.retrieval_mode', 'manual',
    function(value) {
      Services.prefs.setCharPref('dom.mms.retrieval_mode', value);
  });

  SettingsListener.observe('ril.sms.strict7BitEncoding.enabled', false,
    function(value) {
      Services.prefs.setBoolPref('dom.sms.strict7BitEncoding', value);
  });

  SettingsListener.observe('ril.sms.requestStatusReport.enabled', false,
    function(value) {
      Services.prefs.setBoolPref('dom.sms.requestStatusReport', value);
  });

  SettingsListener.observe('ril.mms.requestStatusReport.enabled', false,
    function(value) {
      Services.prefs.setBoolPref('dom.mms.requestStatusReport', value);
  });

  SettingsListener.observe('ril.mms.requestReadReport.enabled', true,
    function(value) {
      Services.prefs.setBoolPref('dom.mms.requestReadReport', value);
  });

  SettingsListener.observe('ril.cellbroadcast.disabled', false,
    function(value) {
      Services.prefs.setBoolPref('ril.cellbroadcast.disabled', value);
  });

  SettingsListener.observe('ril.radio.disabled', false,
    function(value) {
      Services.prefs.setBoolPref('ril.radio.disabled', value);
  });

  SettingsListener.observe('wap.UAProf.url', '',
    function(value) {
      Services.prefs.setCharPref('wap.UAProf.url', value);
  });

  SettingsListener.observe('wap.UAProf.tagname', 'x-wap-profile',
    function(value) {
      Services.prefs.setCharPref('wap.UAProf.tagname', value);
  });

  
  ['mms', 'sms', 'telephony', 'voicemail'].forEach(function(key) {
    SettingsListener.observe('ril.' + key + '.defaultServiceId', 0,
                             function(value) {
      if (value != null) {
        Services.prefs.setIntPref('dom.' + key + '.defaultServiceId', value);
      }
    });
  });
})();


Components.utils.import('resource://gre/modules/XPCOMUtils.jsm');
Components.utils.import('resource://gre/modules/ctypes.jsm');
(function DeviceInfoToSettings() {
  
  
#filter attemptSubstitution
  let os_version = '@MOZ_B2G_VERSION@';
  let os_name = '@MOZ_B2G_OS_NAME@';
#unfilter attemptSubstitution

  let appInfo = Cc["@mozilla.org/xre/app-info;1"]
                  .getService(Ci.nsIXULAppInfo);
  let update_channel = Services.prefs.getCharPref('app.update.channel');

  
  let hardware_info = null;
  let firmware_revision = null;
  let product_model = null;
#ifdef MOZ_WIDGET_GONK
    hardware_info = libcutils.property_get('ro.hardware');
    firmware_revision = libcutils.property_get('ro.firmware_revision');
    product_model = libcutils.property_get('ro.product.model');
#endif

  let software = os_name + ' ' + os_version;
  let setting = {
    'deviceinfo.os': os_version,
    'deviceinfo.software': software,
    'deviceinfo.platform_version': appInfo.platformVersion,
    'deviceinfo.platform_build_id': appInfo.platformBuildID,
    'deviceinfo.update_channel': update_channel,
    'deviceinfo.hardware': hardware_info,
    'deviceinfo.firmware_revision': firmware_revision,
    'deviceinfo.product_model': product_model
  }
  window.navigator.mozSettings.createLock().set(setting);
})();



let devtoolsWidgetPanel;
SettingsListener.observe('devtools.overlay', false, (value) => {
  if (value) {
    if (!devtoolsWidgetPanel) {
      let scope = {};
      Services.scriptloader.loadSubScript('chrome://b2g/content/devtools.js', scope);
      devtoolsWidgetPanel = scope.devtoolsWidgetPanel;
    }
    devtoolsWidgetPanel.init();
  } else {
    if (devtoolsWidgetPanel) {
      devtoolsWidgetPanel.uninit();
    }
  }
});



#ifdef MOZ_WIDGET_GONK
let AdbController = {
  DEBUG: false,
  locked: undefined,
  remoteDebuggerEnabled: undefined,
  lockEnabled: undefined,
  disableAdbTimer: null,
  disableAdbTimeoutHours: 12,
  umsActive: false,

  debug: function(str) {
    dump("AdbController: " + str + "\n");
  },

  setLockscreenEnabled: function(value) {
    this.lockEnabled = value;
    if (this.DEBUG) {
      this.debug("setLockscreenEnabled = " + this.lockEnabled);
    }
    this.updateState();
  },

  setLockscreenState: function(value) {
    this.locked = value;
    if (this.DEBUG) {
      this.debug("setLockscreenState = " + this.locked);
    }
    this.updateState();
  },

  setRemoteDebuggerState: function(value) {
    this.remoteDebuggerEnabled = value;
    if (this.DEBUG) {
      this.debug("setRemoteDebuggerState = " + this.remoteDebuggerEnabled);
    }
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
      if (this.DEBUG) {
        this.debug("Timer to disable ADB not started due to zero timeout");
      }
      return;
    }

    if (this.DEBUG) {
      this.debug("Starting timer to disable ADB in " +
                 this.disableAdbTimeoutHours + " hours");
    }
    let timeoutMilliseconds = this.disableAdbTimeoutHours * 60 * 60 * 1000;
    this.disableAdbTimer.initWithCallback(this, timeoutMilliseconds,
                                          Ci.nsITimer.TYPE_ONE_SHOT);
  },

  stopDisableAdbTimer: function() {
    if (this.DEBUG) {
      this.debug("Stopping timer to disable ADB");
    }
    if (this.disableAdbTimer) {
      this.disableAdbTimer.cancel();
      this.disableAdbTimer = null;
    }
  },

  notify: function(aTimer) {
    if (aTimer == this.disableAdbTimer) {
      this.disableAdbTimer = null;
      
      
      
      dump("AdbController: ADB timer expired - disabling ADB\n");
      navigator.mozSettings.createLock().set(
        {'devtools.debugger.remote-enabled': false});
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
    if (this.DEBUG) {
      this.debug("Checking availability of storage: '" +
                 storage.storageName);
    }

    let req = storage.available();
    req.onsuccess = function(e) {
      if (this.DEBUG) {
        this.debug("Storage: '" + storage.storageName + "' is '" +
                   e.target.result);
      }
      if (e.target.result == 'shared') {
        
        
        this.umsActive = true;
        this.updateStateInternal();
        return;
      }
      this.updateStorageState(storageIndex + 1);
    }.bind(this);
    req.onerror = function(e) {
      dump("AdbController: error querying storage availability for '" +
           this.storages[storageIndex].storageName + "' (ignoring)\n");
      this.updateStorageState(storageIndex + 1);
    }.bind(this);
  },

  updateStateInternal: function() {
    if (this.DEBUG) {
      this.debug("updateStateInternal: called");
    }

    if (this.remoteDebuggerEnabled === undefined ||
        this.lockEnabled === undefined ||
        this.locked === undefined) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      if (this.DEBUG) {
        this.debug("updateState: Waiting for all vars to be initialized");
      }
      return;
    }

    
    
    let isDebugging = RemoteDebugger.isDebugging;
    if (this.DEBUG) {
      this.debug("isDebugging=" + isDebugging);
    }

    
    
    
    let sysUsbConfig = libcutils.property_get("sys.usb.config");
    let rndisActive = (sysUsbConfig.split(",").indexOf("rndis") >= 0);
    let usbFuncActive = rndisActive || this.umsActive || isDebugging;

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
    if (this.DEBUG) {
      this.debug("updateState: enableAdb = " + enableAdb +
                 " remoteDebuggerEnabled = " + this.remoteDebuggerEnabled +
                 " lockEnabled = " + this.lockEnabled +
                 " locked = " + this.locked +
                 " usbFuncActive = " + usbFuncActive);
    }

    
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
      if (this.DEBUG) {
        this.debug("updateState: currentConfig = " + currentConfig);
        this.debug("updateState:     newConfig = " + newConfig);
      }
      try {
        libcutils.property_set("persist.sys.usb.config", newConfig);
      } catch(e) {
        dump("Error configuring adb: " + e);
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
#endif



SettingsListener.observe('devtools.debugger.remote-enabled', false, function(value) {
  Services.prefs.setBoolPref('devtools.debugger.remote-enabled', value);
  
  Services.prefs.savePrefFile(null);
  try {
    value ? RemoteDebugger.start() : RemoteDebugger.stop();
  } catch(e) {
    dump("Error while initializing devtools: " + e + "\n" + e.stack + "\n");
  }

#ifdef MOZ_WIDGET_GONK
  AdbController.setRemoteDebuggerState(value);
#endif
});

SettingsListener.observe('debugger.remote-mode', false, function(value) {
  if (['disabled', 'adb-only', 'adb-devtools'].indexOf(value) == -1) {
    dump('Illegal value for debugger.remote-mode: ' + value + '\n');
    return;
  }

  Services.prefs.setBoolPref('devtools.debugger.remote-enabled',
                             value == 'adb-devtools');
  
  Services.prefs.savePrefFile(null);

  try {
    (value == 'adb-devtools') ? RemoteDebugger.start()
                              : RemoteDebugger.stop();
  } catch(e) {
    dump("Error while initializing devtools: " + e + "\n" + e.stack + "\n");
  }

#ifdef MOZ_WIDGET_GONK
  AdbController.setRemoteDebuggerState(value != 'disabled');
#endif
});


SettingsListener.observe('debug.log-animations.enabled', false, function(value) {
  Services.prefs.setBoolPref('layers.offmainthreadcomposition.log-animations', value);
});


SettingsListener.observe('device.storage.writable.name', 'sdcard', function(value) {
  if (Services.prefs.getPrefType('device.storage.writable.name') != Ci.nsIPrefBranch.PREF_STRING) {
    
    
    Services.prefs.clearUserPref('device.storage.writable.name');
  }
  Services.prefs.setCharPref('device.storage.writable.name', value);
});


SettingsListener.observe('privacy.donottrackheader.enabled', false, function(value) {
  Services.prefs.setBoolPref('privacy.donottrackheader.enabled', value);
});

SettingsListener.observe('privacy.donottrackheader.value', 1, function(value) {
  Services.prefs.setIntPref('privacy.donottrackheader.value', value);
  
  
  if (value == 1) {
    Services.prefs.setCharPref('app.update.custom', '');
    return;
  }
  
  setUpdateTrackingId();
});


SettingsListener.observe('app.reportCrashes', 'ask', function(value) {
  if (value == 'always') {
    Services.prefs.setBoolPref('app.reportCrashes', true);
  } else if (value == 'never') {
    Services.prefs.setBoolPref('app.reportCrashes', false);
  } else {
    Services.prefs.clearUserPref('app.reportCrashes');
  }
  
  Services.prefs.savePrefFile(null);
});








function setUpdateTrackingId() {
  try {
    let dntEnabled = Services.prefs.getBoolPref('privacy.donottrackheader.enabled');
    let dntValue =  Services.prefs.getIntPref('privacy.donottrackheader.value');
    
    if (dntEnabled && (dntValue == 1)) {
      return;
    }

    let trackingId =
      Services.prefs.getPrefType('app.update.custom') ==
      Ci.nsIPrefBranch.PREF_STRING &&
      Services.prefs.getCharPref('app.update.custom');

    
    
    
    if (!trackingId) {
      trackingId = uuidgen.generateUUID().toString().replace(/[{}]/g, "");
      Services.prefs.setCharPref('app.update.custom', trackingId);
    }
  } catch(e) {
    dump('Error getting tracking ID ' + e + '\n');
  }
}
setUpdateTrackingId();

SettingsListener.observe('app.update.interval', 86400, function(value) {
  Services.prefs.setIntPref('app.update.interval', value);
});



SettingsListener.observe("debug.fps.enabled", false, function(value) {
  Services.prefs.setBoolPref("layers.acceleration.draw-fps", value);
});
SettingsListener.observe("debug.paint-flashing.enabled", false, function(value) {
  Services.prefs.setBoolPref("nglayout.debug.paint_flashing", value);
});
SettingsListener.observe("layers.draw-borders", false, function(value) {
  Services.prefs.setBoolPref("layers.draw-borders", value);
});

(function Composer2DSettingToPref() {
  
  
  
  
  
  

  var req = navigator.mozSettings.createLock().get('layers.composer2d.enabled');
  req.onsuccess = function() {
    if (typeof(req.result['layers.composer2d.enabled']) === 'undefined') {
      var enabled = false;
      if (Services.prefs.getPrefType('layers.composer2d.enabled') == Ci.nsIPrefBranch.PREF_BOOL) {
        enabled = Services.prefs.getBoolPref('layers.composer2d.enabled');
      } else {
#ifdef MOZ_WIDGET_GONK
        enabled = (libcutils.property_get('ro.display.colorfill') === '1');
#endif
      }
      navigator.mozSettings.createLock().set({'layers.composer2d.enabled': enabled });
    }

    SettingsListener.observe("layers.composer2d.enabled", true, function(value) {
      Services.prefs.setBoolPref("layers.composer2d.enabled", value);
    });
  };
  req.onerror = function() {
    dump("Error configuring layers.composer2d.enabled setting");
  };

})();


SettingsListener.observe("accessibility.screenreader", false, function(value) {
  if (value && !("AccessFu" in this)) {
    Cu.import('resource://gre/modules/accessibility/AccessFu.jsm');
    AccessFu.attach(window);
  }
});


(function themingSettingsListener() {
  let themingPrefs = ['ui.menu', 'ui.menutext', 'ui.infobackground', 'ui.infotext',
                      'ui.window', 'ui.windowtext', 'ui.highlight'];

  themingPrefs.forEach(function(pref) {
    SettingsListener.observe('gaia.' + pref, null, function(value) {
      if (value) {
        Services.prefs.setCharPref(pref, value);
      }
    });
  });
})();



SettingsListener.observe('apz.force-enable', false, function(value) {
  Services.prefs.setBoolPref('dom.browser_frames.useAsyncPanZoom', value);
});

SettingsListener.observe('apz.displayport.heuristics', 'default', function(value) {
  
  Services.prefs.setCharPref('apz.velocity_bias', '1.0');
  Services.prefs.setBoolPref('apz.use_paint_duration', true);
  Services.prefs.setCharPref('apz.x_skate_size_multiplier', '1.5');
  Services.prefs.setCharPref('apz.y_skate_size_multiplier', '2.5');
  Services.prefs.setBoolPref('apz.allow-checkerboarding', true);
  
  switch (value) {
  case 'default':
    break;
  case 'center-displayport':
    Services.prefs.setCharPref('apz.velocity_bias', '0.0');
    break;
  case 'perfect-paint-times':
    Services.prefs.setBoolPref('apz.use_paint_duration', false);
    Services.prefs.setCharPref('apz.velocity_bias', '0.32'); 
    break;
  case 'taller-displayport':
    Services.prefs.setCharPref('apz.y_skate_size_multiplier', '3.5');
    break;
  case 'faster-paint':
    Services.prefs.setCharPref('apz.x_skate_size_multiplier', '1.0');
    Services.prefs.setCharPref('apz.y_skate_size_multiplier', '1.5');
    break;
  case 'no-checkerboard':
    Services.prefs.setBoolPref('apz.allow-checkerboarding', false);
    break;
  }
});

SettingsListener.observe('layers.enable-tiles', false, function(value) {
  Services.prefs.setBoolPref('layers.enable-tiles', value);
});

SettingsListener.observe('layers.progressive-paint', false, function(value) {
  Services.prefs.setBoolPref('layers.progressive-paint', value);
});

SettingsListener.observe('layers.draw-tile-borders', false, function(value) {
  Services.prefs.setBoolPref('layers.draw-tile-borders', value);
});

