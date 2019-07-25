





"use strict;"



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

    var req = settings.getLock().get(name);
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
  Services.prefs.setCharPref('intl.accept_languages', value);
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



SettingsListener.observe('devtools.debugger.remote-enabled', false, function(enabled) {
  Services.prefs.setBoolPref('devtools.debugger.remote-enabled', value);
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
