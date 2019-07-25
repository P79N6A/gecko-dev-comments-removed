





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



SettingsListener.observe('language.current', 'en-US', function(value) {
  Services.prefs.setCharPref('intl.accept_languages', value);
});



(function RILSettingsToPrefs() {
  ['ril.data.enabled', 'ril.data.roaming.enabled'].forEach(function(key) {
    SettingsListener.observe(key, false, function(value) {
      Services.prefs.setBoolPref(key, value);
    });
  });

  let strPrefs = ['ril.data.apn', 'ril.data.user', 'ril.data.passwd',
                  'ril.data.mmsc', 'ril.data.mmsproxy'];
  strPrefs.forEach(function(key) {
    SettingsListener.observe(key, false, function(value) {
      Services.prefs.setCharPref(key, value);
    });
  });

  ['ril.data.mmsport'].forEach(function(key) {
    SettingsListener.observe(key, false, function(value) {
      Services.prefs.setIntPref(key, value);
    });
  });
})();



SettingsListener.observe('devtools.debugger.enabled', false, function(enabled) {
  Services.prefs.setBoolPref('devtools.debugger.enabled', value);
});

SettingsListener.observe('devtools.debugger.log', false, function(value) {
  Services.prefs.setBoolPref('devtools.debugger.log', value);
});

SettingsListener.observe('devtools.debugger.port', 6000, function(value) {
  Services.prefs.setIntPref('devtools.debugger.port', value);
});

