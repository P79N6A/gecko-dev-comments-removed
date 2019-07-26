





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
})();


Components.utils.import('resource://gre/modules/XPCOMUtils.jsm');
Components.utils.import('resource://gre/modules/ctypes.jsm');
(function DeviceInfoToSettings() {
  XPCOMUtils.defineLazyServiceGetter(this, 'gSettingsService',
                                     '@mozilla.org/settingsService;1',
                                     'nsISettingsService');
  let lock = gSettingsService.createLock();
  
  
#filter attemptSubstitution
  let os_version = '@MOZ_B2G_VERSION@';
  let os_name = '@MOZ_B2G_OS_NAME@';
#unfilter attemptSubstitution
  lock.set('deviceinfo.os', os_version, null, null);
  lock.set('deviceinfo.software', os_name + ' ' + os_version, null, null);

  let appInfo = Cc["@mozilla.org/xre/app-info;1"]
                  .getService(Ci.nsIXULAppInfo);
  lock.set('deviceinfo.platform_version', appInfo.platformVersion, null, null);
  lock.set('deviceinfo.platform_build_id', appInfo.platformBuildID, null, null);

  let update_channel = Services.prefs.getCharPref('app.update.channel');
  lock.set('deviceinfo.update_channel', update_channel, null, null);

  
  let hardware_info = null;
  let firmware_revision = null;
  let product_model = null;
#ifdef MOZ_WIDGET_GONK
    hardware_info = libcutils.property_get('ro.hardware');
    firmware_revision = libcutils.property_get('ro.firmware_revision');
    product_model = libcutils.property_get('ro.product.model');
#endif
  lock.set('deviceinfo.hardware', hardware_info, null, null);
  lock.set('deviceinfo.firmware_revision', firmware_revision, null, null);
  lock.set('deviceinfo.product_model', product_model, null, null);
})();


SettingsListener.observe('devtools.debugger.remote-enabled', false, function(value) {
  Services.prefs.setBoolPref('devtools.debugger.remote-enabled', value);
  
  Services.prefs.savePrefFile(null);
  try {
    value ? RemoteDebugger.start() : RemoteDebugger.stop();
  } catch(e) {
    dump("Error while initializing devtools: " + e + "\n" + e.stack + "\n");
  }

#ifdef MOZ_WIDGET_GONK
  let enableAdb = value;

  try {
    if (Services.prefs.getBoolPref('marionette.defaultPrefs.enabled')) {
      
      
      

      enableAdb = true;
    }
  } catch (e) {
    
    
  }

  
  try {
    let currentConfig = libcutils.property_get("persist.sys.usb.config");
    let configFuncs = currentConfig.split(",");
    let adbIndex = configFuncs.indexOf("adb");

    if (enableAdb) {
      
      if (adbIndex < 0) {
        configFuncs.push("adb");
      }
    } else {
      
      if (adbIndex >= 0) {
        configFuncs.splice(adbIndex,1);
      }
    }
    let newConfig = configFuncs.join(",");
    if (newConfig != currentConfig) {
      libcutils.property_set("persist.sys.usb.config", newConfig);
    }
  } catch(e) {
    dump("Error configuring adb: " + e);
  }
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


SettingsListener.observe('app.reportCrashes', 'ask', function(value) {
  if (value == 'always') {
    Services.prefs.setBoolPref('app.reportCrashes', true);
  } else if (value == 'never') {
    Services.prefs.setBoolPref('app.reportCrashes', false);
  } else {
    Services.prefs.clearUserPref('app.reportCrashes');
  }
});


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
