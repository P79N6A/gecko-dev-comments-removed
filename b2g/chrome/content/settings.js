





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
    'deviceinfo.hardware': hardware_info,
    'deviceinfo.firmware_revision': firmware_revision,
    'deviceinfo.product_model': product_model
  }
  window.navigator.mozSettings.createLock().set(setting);
})();



let developerHUD;
SettingsListener.observe('devtools.overlay', false, (value) => {
  if (value) {
    if (!developerHUD) {
      let scope = {};
      Services.scriptloader.loadSubScript('chrome://b2g/content/devtools.js', scope);
      developerHUD = scope.developerHUD;
    }
    developerHUD.init();
  } else {
    if (developerHUD) {
      developerHUD.uninit();
    }
  }
});


SettingsListener.observe('device.storage.writable.name', 'sdcard', function(value) {
  if (Services.prefs.getPrefType('device.storage.writable.name') != Ci.nsIPrefBranch.PREF_STRING) {
    
    
    Services.prefs.clearUserPref('device.storage.writable.name');
  }
  Services.prefs.setCharPref('device.storage.writable.name', value);
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


SettingsListener.observe('apz.displayport.heuristics', 'default', function(value) {
  
  Services.prefs.clearUserPref('apz.velocity_bias');
  Services.prefs.clearUserPref('apz.use_paint_duration');
  Services.prefs.clearUserPref('apz.x_skate_size_multiplier');
  Services.prefs.clearUserPref('apz.y_skate_size_multiplier');
  Services.prefs.clearUserPref('apz.allow-checkerboarding');
  
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


let settingsToObserve = {
  'ril.mms.retrieval_mode': {
    prefName: 'dom.mms.retrieval_mode',
    defaultValue: 'manual'
  },
  'ril.sms.strict7BitEncoding.enabled': {
    prefName: 'dom.sms.strict7BitEncoding',
    defaultValue: false
  },
  'ril.sms.requestStatusReport.enabled': {
    prefName: 'dom.sms.requestStatusReport',
    defaultValue: false
  },
  'ril.mms.requestStatusReport.enabled': {
    prefName: 'dom.mms.requestStatusReport',
    defaultValue: false
  },
  'ril.mms.requestReadReport.enabled': {
    prefName: 'dom.mms.requestReadReport',
    defaultValue: true
  },
  'ril.cellbroadcast.disabled': false,
  'ril.radio.disabled': false,
  'wap.UAProf.url': '',
  'wap.UAProf.tagname': 'x-wap-profile',
  'devtools.eventlooplag.threshold': 100,
  'privacy.donottrackheader.enabled': false,
  'apz.force-enable': {
    prefName: 'dom.browser_frames.useAsyncPanZoom',
    defaultValue: false
  },
  'layers.enable-tiles': true,
  'layers.simple-tiles': false,
  'layers.progressive-paint': false,
  'layers.draw-tile-borders': false,
  'layers.dump': false,
  'debug.fps.enabled': {
    prefName: 'layers.acceleration.draw-fps',
    defaultValue: false
  },
  'debug.paint-flashing.enabled': {
    prefName: 'nglayout.debug.paint_flashing',
    defaultValue: false
  },
  'layers.draw-borders': false,
  'app.update.interval': 86400,
  'app.update.url': {
    resetToPref: true
  },
  'app.update.channel': {
    resetToPref: true
  },
  'debug.log-animations.enabled': {
    prefName: 'layers.offmainthreadcomposition.log-animations',
    defaultValue: false
  }
};

for (let key in settingsToObserve) {
  let setting = settingsToObserve[key];

  
  let prefName = setting.prefName || key;
  let defaultValue = setting.defaultValue;
  if (defaultValue === undefined) {
    defaultValue = setting;
  }

  let prefs = Services.prefs;

  
  if (setting.resetToPref) {
    switch (prefs.getPrefType(prefName)) {
      case Ci.nsIPrefBranch.PREF_BOOL:
        defaultValue = prefs.getBoolPref(prefName);
        break;

      case Ci.nsIPrefBranch.PREF_INT:
        defaultValue = prefs.getIntPref(prefName);
        break;

      case Ci.nsIPrefBranch.PREF_STRING:
        defaultValue = prefs.getCharPref(prefName);
        break;
    }

    let setting = {};
    setting[key] = defaultValue;
    window.navigator.mozSettings.createLock().set(setting);
  }

  
  let setPref;
  switch (typeof defaultValue) {
    case 'boolean':
      setPref = prefs.setBoolPref.bind(prefs);
      break;

    case 'number':
      setPref = prefs.setIntPref.bind(prefs);
      break;

    case 'string':
      setPref = prefs.setCharPref.bind(prefs);
      break;
  }

  SettingsListener.observe(key, defaultValue, function(value) {
    setPref(prefName, value);
  });
};

