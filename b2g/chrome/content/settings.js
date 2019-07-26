





"use strict;"

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');



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


SettingsListener.observe('audio.volume.master', 0.5, function(value) {
  let audioManager = Services.audioManager;
  if (!audioManager)
    return;

  audioManager.masterVolume = Math.max(0.0, Math.min(value, 1.0));
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
    Services.prefs.setCharPref(prefName, value + ', ' + intl);
  }
});



(function RILSettingsToPrefs() {
  let strPrefs = ['ril.data.mmsc', 'ril.data.mmsproxy'];
  strPrefs.forEach(function(key) {
    SettingsListener.observe(key, "", function(value) {
      Services.prefs.setCharPref(key, value);
    });
  });

  ['ril.data.mmsport'].forEach(function(key) {
    SettingsListener.observe(key, null, function(value) {
      if (value != null) {
        Services.prefs.setIntPref(key, value);
      }
    });
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
#unfilter attemptSubstitution
  lock.set('deviceinfo.os', os_version, null, null);

  let appInfo = Cc["@mozilla.org/xre/app-info;1"]
                  .getService(Ci.nsIXULAppInfo);
  lock.set('deviceinfo.platform_version', appInfo.platformVersion, null, null);
  lock.set('deviceinfo.platform_build_id', appInfo.platformBuildID, null, null);

  let update_channel = Services.prefs.getCharPref('app.update.channel');
  lock.set('deviceinfo.update_channel', update_channel, null, null);

  
  let hardware_version = null;
  try {
    let cutils = ctypes.open('libcutils.so');
    let cbuf = ctypes.char.array(128)();
    let c_property_get = cutils.declare('property_get', ctypes.default_abi,
                                        ctypes.int,       
                                        ctypes.char.ptr,  
                                        ctypes.char.ptr,  
                                        ctypes.char.ptr); 
    let property_get = function (key, defaultValue) {
      if (defaultValue === undefined) {
        defaultValue = null;
      }
      c_property_get(key, cbuf, defaultValue);
      return cbuf.readString();
    }
    hardware_version = property_get('ro.hardware');
    cutils.close();
  } catch(e) {
    
  }
  lock.set('deviceinfo.hardware', hardware_version, null, null);
})();


SettingsListener.observe('devtools.debugger.remote-enabled', false, function(value) {
  Services.prefs.setBoolPref('devtools.debugger.remote-enabled', value);
  
  Services.prefs.savePrefFile(null);
});

SettingsListener.observe('devtools.debugger.log', false, function(value) {
  Services.prefs.setBoolPref('devtools.debugger.log', value);
});

SettingsListener.observe('devtools.debugger.remote-port', 6000, function(value) {
  Services.prefs.setIntPref('devtools.debugger.remote-port', value);
});

SettingsListener.observe('devtools.debugger.force-local', true, function(value) {
  Services.prefs.setBoolPref('devtools.debugger.force-local', value);
});

SettingsListener.observe('debug.log-animations.enabled', false, function(value) {
  Services.prefs.setBoolPref('layers.offmainthreadcomposition.log-animations', value);
});

SettingsListener.observe('debug.dev-mode', false, function(value) {
  Services.prefs.setBoolPref('dom.mozApps.dev_mode', value);
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

