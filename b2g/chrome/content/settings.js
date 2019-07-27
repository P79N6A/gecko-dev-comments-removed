





"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;





Cu.import('resource://gre/modules/SettingsRequestManager.jsm');
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

XPCOMUtils.defineLazyServiceGetter(this, "gPACGenerator",
                                   "@mozilla.org/pac-generator;1",
                                   "nsIPACGenerator");



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

SettingsListener.observe('homescreen.manifestURL', 'Sentinel Value' , function(value) {
  Services.prefs.setCharPref('dom.mozApps.homescreenURL', value);
});


SettingsListener.observe('language.current', 'en-US', function(value) {
  Services.prefs.setCharPref('general.useragent.locale', value);

  let prefName = 'intl.accept_languages';
  let defaultBranch = Services.prefs.getDefaultBranch(null);

  let intl = '';
  try {
    intl = defaultBranch.getComplexValue(prefName,
                                         Ci.nsIPrefLocalizedString).data;
  } catch(e) {}

  
  
  
  
  
  if (!((new RegExp('^' + value + '[^a-z-_] *[,;]?', 'i')).test(intl))) {
    value = value + ', ' + intl;
  } else {
    value = intl;
  }
  Services.prefs.setCharPref(prefName, value);

  if (shell.hasStarted() == false) {
    shell.bootstrap();
  }
});


(function RILSettingsToPrefs() {
  
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
  let build_number = null;
#ifdef MOZ_WIDGET_GONK
    hardware_info = libcutils.property_get('ro.hardware');
    firmware_revision = libcutils.property_get('ro.firmware_revision');
    product_model = libcutils.property_get('ro.product.model');
    build_number = libcutils.property_get('ro.build.version.incremental');
#endif

  
  
  let lock = window.navigator.mozSettings.createLock();
  let req = lock.get('deviceinfo.os');
  req.onsuccess = req.onerror = () => {
    let previous_os = req.result && req.result['deviceinfo.os'] || '';
    let software = os_name + ' ' + os_version;
    let setting = {
      'deviceinfo.build_number': build_number,
      'deviceinfo.os': os_version,
      'deviceinfo.previous_os': previous_os,
      'deviceinfo.software': software,
      'deviceinfo.platform_version': appInfo.platformVersion,
      'deviceinfo.platform_build_id': appInfo.platformBuildID,
      'deviceinfo.hardware': hardware_info,
      'deviceinfo.firmware_revision': firmware_revision,
      'deviceinfo.product_model': product_model
    }
    lock.set(setting);
  }
})();



let developerHUD;
SettingsListener.observe('devtools.overlay', false, (value) => {
  if (value) {
    if (!developerHUD) {
      let scope = {};
      Services.scriptloader.loadSubScript('chrome://b2g/content/devtools/hud.js', scope);
      developerHUD = scope.developerHUD;
    }
    developerHUD.init();
  } else {
    if (developerHUD) {
      developerHUD.uninit();
    }
  }
});

#ifdef MOZ_WIDGET_GONK

let LogShake;
(function() {
  let scope = {};
  Cu.import('resource://gre/modules/LogShake.jsm', scope);
  LogShake = scope.LogShake;
  LogShake.init();
})();

SettingsListener.observe('devtools.logshake', false, value => {
  if (value) {
    LogShake.enableDeviceMotionListener();
  } else {
    LogShake.disableDeviceMotionListener();
  }
});
#endif


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

(function syncUpdatePrefs() {
  
  
  
  let defaultBranch = Services.prefs.getDefaultBranch(null);

  function syncCharPref(prefName) {
    SettingsListener.observe(prefName, null, function(value) {
      
      if (value) {
        defaultBranch.setCharPref(prefName, value);
        return;
      }
      
      try {
        let value = defaultBranch.getCharPref(prefName);
        if (value) {
          let setting = {};
          setting[prefName] = value;
          window.navigator.mozSettings.createLock().set(setting);
        }
      } catch(e) {
        console.log('Unable to read pref ' + prefName + ': ' + e);
      }
    });
  }

  syncCharPref('app.update.url');
  syncCharPref('app.update.channel');
})();


(function Composer2DSettingToPref() {
  
  
  
  
  
  

  var req = navigator.mozSettings.createLock().get('layers.composer2d.enabled');
  req.onsuccess = function() {
    if (typeof(req.result['layers.composer2d.enabled']) === 'undefined') {
      var enabled = false;
      if (Services.prefs.getPrefType('layers.composer2d.enabled') == Ci.nsIPrefBranch.PREF_BOOL) {
        enabled = Services.prefs.getBoolPref('layers.composer2d.enabled');
      } else {
#ifdef MOZ_WIDGET_GONK
        let androidVersion = libcutils.property_get("ro.build.version.sdk");
        if (androidVersion >= 17 ) {
          enabled = true;
        } else {
          enabled = (libcutils.property_get('ro.display.colorfill') === '1');
        }
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


(function setupAccessibility() {
  let accessibilityScope = {};
  SettingsListener.observe("accessibility.screenreader", false, function(value) {
    if (!value) {
      return;
    }
    if (!('AccessFu' in accessibilityScope)) {
      Cu.import('resource://gre/modules/accessibility/AccessFu.jsm',
                accessibilityScope);
      accessibilityScope.AccessFu.attach(window);
    }
  });
})();


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


(function setupTelemetrySettings() {
  let gaiaSettingName = 'debug.performance_data.shared';
  let geckoPrefName = 'toolkit.telemetry.enabled';
  SettingsListener.observe(gaiaSettingName, null, function(value) {
    if (value !== null) {
      
      Services.prefs.setBoolPref(geckoPrefName, value);
      return;
    }
    
#ifdef MOZ_TELEMETRY_ON_BY_DEFAULT
    let prefValue = true;
#else
    let prefValue = false;
#endif
    try {
      prefValue = Services.prefs.getBoolPref(geckoPrefName);
    } catch (e) {
      
    }
    let setting = {};
    setting[gaiaSettingName] = prefValue;
    window.navigator.mozSettings.createLock().set(setting);
  });
})();


(function setupLowPrecisionSettings() {
  
  SettingsListener.observe('layers.low-precision', null, function(value) {
    if (value !== null) {
      
      Services.prefs.setBoolPref('layers.low-precision-buffer', value);
      Services.prefs.setBoolPref('layers.progressive-paint', value);
    } else {
      
      try {
        let prefValue = Services.prefs.getBoolPref('layers.low-precision-buffer');
        let setting = { 'layers.low-precision': prefValue };
        window.navigator.mozSettings.createLock().set(setting);
      } catch (e) {
        console.log('Unable to read pref layers.low-precision-buffer: ' + e);
      }
    }
  });

  
  SettingsListener.observe('layers.low-opacity', null, function(value) {
    if (value !== null) {
      
      Services.prefs.setCharPref('layers.low-precision-opacity', value ? '0.5' : '1.0');
    } else {
      
      try {
        let prefValue = Services.prefs.getCharPref('layers.low-precision-opacity');
        let setting = { 'layers.low-opacity': (prefValue == '0.5') };
        window.navigator.mozSettings.createLock().set(setting);
      } catch (e) {
        console.log('Unable to read pref layers.low-precision-opacity: ' + e);
      }
    }
  });
})();



SettingsListener.observe("theme.selected",
                         "app://default_theme.gaiamobile.org/manifest.webapp",
                         function(value) {
  if (!value) {
    return;
  }

  let newTheme;
  try {
    let enabled = Services.prefs.getBoolPref("dom.mozApps.themable");
    if (!enabled) {
      return;
    }

    
    let uri = Services.io.newURI(value, null, null);
    
    if (uri.scheme !== "app") {
      return;
    }
    newTheme = uri.host;
  } catch(e) {
    return;
  }

  let currentTheme;
  try {
    currentTheme = Services.prefs.getCharPref('dom.mozApps.selected_theme');
  } catch(e) {};

  if (currentTheme != newTheme) {
    debug("New theme selected " + value);
    Services.prefs.setCharPref('dom.mozApps.selected_theme', newTheme);
    Services.prefs.savePrefFile(null);
    Services.obs.notifyObservers(null, 'app-theme-changed', newTheme);
  }
});


(function setupBrowsingProxySettings() {
  function setPAC() {
    let usePAC;
    try {
      usePAC = Services.prefs.getBoolPref('network.proxy.pac_generator');
    } catch (ex) {}

    if (usePAC) {
      Services.prefs.setCharPref('network.proxy.autoconfig_url',
                                 gPACGenerator.generate());
      Services.prefs.setIntPref('network.proxy.type',
                                Ci.nsIProtocolProxyService.PROXYCONFIG_PAC);
    }
  }

  SettingsListener.observe('browser.proxy.enabled', false, function(value) {
    Services.prefs.setBoolPref('network.proxy.browsing.enabled', value);
    setPAC();
  });

  SettingsListener.observe('browser.proxy.host', '', function(value) {
    Services.prefs.setCharPref('network.proxy.browsing.host', value);
    setPAC();
  });

  SettingsListener.observe('browser.proxy.port', 0, function(value) {
    Services.prefs.setIntPref('network.proxy.browsing.port', value);
    setPAC();
  });

  setPAC();
})();


let settingsToObserve = {
  'accessibility.screenreader_quicknav_modes': {
    prefName: 'accessibility.accessfu.quicknav_modes',
    resetToPref: true,
    defaultValue: ''
  },
  'accessibility.screenreader_quicknav_index': {
    prefName: 'accessibility.accessfu.quicknav_index',
    resetToPref: true,
    defaultValue: 0
  },
  'app.update.interval': 86400,
  'apz.overscroll.enabled': true,
  'debug.fps.enabled': {
    prefName: 'layers.acceleration.draw-fps',
    defaultValue: false
  },
  'debug.log-animations.enabled': {
    prefName: 'layers.offmainthreadcomposition.log-animations',
    defaultValue: false
  },
  'debug.paint-flashing.enabled': {
    prefName: 'nglayout.debug.paint_flashing',
    defaultValue: false
  },
  'devtools.eventlooplag.threshold': 100,
  'devtools.remote.wifi.visible': {
    resetToPref: true
  },
  'dom.mozApps.use_reviewer_certs': false,
  'dom.mozApps.signed_apps_installable_from': 'https://marketplace.firefox.com',
  'gfx.layerscope.enabled': false,
  'layers.draw-borders': false,
  'layers.draw-tile-borders': false,
  'layers.dump': false,
  'layers.enable-tiles': true,
  'layers.effect.invert': false,
  'layers.effect.grayscale': false,
  'layers.effect.contrast': "0.0",
  'network.debugging.enabled': false,
  'privacy.donottrackheader.enabled': false,
  'ril.debugging.enabled': false,
  'ril.radio.disabled': false,
  'ril.mms.requestReadReport.enabled': {
    prefName: 'dom.mms.requestReadReport',
    defaultValue: true
  },
  'ril.mms.requestStatusReport.enabled': {
    prefName: 'dom.mms.requestStatusReport',
    defaultValue: false
  },
  'ril.mms.retrieval_mode': {
    prefName: 'dom.mms.retrieval_mode',
    defaultValue: 'manual'
  },
  'ril.sms.requestStatusReport.enabled': {
    prefName: 'dom.sms.requestStatusReport',
    defaultValue: false
  },
  'ril.sms.strict7BitEncoding.enabled': {
    prefName: 'dom.sms.strict7BitEncoding',
    defaultValue: false
  },
  'ril.sms.maxReadAheadEntries': {
    prefName: 'dom.sms.maxReadAheadEntries',
    defaultValue: 7
  },
  'ui.touch.radius.leftmm': {
    resetToPref: true
  },
  'ui.touch.radius.topmm': {
    resetToPref: true
  },
  'ui.touch.radius.rightmm': {
    resetToPref: true
  },
  'ui.touch.radius.bottommm': {
    resetToPref: true
  },
  'wap.UAProf.tagname': 'x-wap-profile',
  'wap.UAProf.url': ''
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
