















'use strict';

var util = require('./util/util');





var settings = {};




var settingValues = {};




var settingStorage;




var types;




exports.setDefaults = function(newValues) {
  Object.keys(newValues).forEach(function(name) {
    if (settingValues[name] === undefined) {
      settingValues[name] = newValues[name];
    }
  });
};




exports.startup = function(t) {
  types = t;
  settingStorage = new LocalSettingStorage();
  settingStorage.load(settingValues);
};

exports.shutdown = function() {
};




exports.getAll = function(filter) {
  var all = [];
  Object.keys(settings).forEach(function(name) {
    if (filter == null || name.indexOf(filter) !== -1) {
      all.push(settings[name]);
    }
  }.bind(this));
  all.sort(function(s1, s2) {
    return s1.name.localeCompare(s2.name);
  }.bind(this));
  return all;
};





exports.addSetting = function(prefSpec) {
  var type = types.createType(prefSpec.type);
  var setting = new Setting(prefSpec.name, type, prefSpec.description,
                            prefSpec.defaultValue);
  settings[setting.name] = setting;
  exports.onChange({ added: setting.name });
  return setting;
};










exports.getSetting = function(name) {
  return settings[name];
};




exports.removeSetting = function(nameOrSpec) {
  var name = typeof nameOrSpec === 'string' ? nameOrSpec : nameOrSpec.name;
  delete settings[name];
  exports.onChange({ removed: name });
};




exports.onChange = util.createEvent('Settings.onChange');





function LocalSettingStorage() {
}

LocalSettingStorage.prototype.load = function(values) {
  if (typeof localStorage === 'undefined') {
    return;
  }

  var gcliSettings = localStorage.getItem('gcli-settings');
  if (gcliSettings != null) {
    var parsed = JSON.parse(gcliSettings);
    Object.keys(parsed).forEach(function(name) {
      values[name] = parsed[name];
    });
  }
};

LocalSettingStorage.prototype.save = function(values) {
  if (typeof localStorage !== 'undefined') {
    var json = JSON.stringify(values);
    localStorage.setItem('gcli-settings', json);
  }
};

exports.LocalSettingStorage = LocalSettingStorage;






function Setting(name, type, description, defaultValue) {
  this.name = name;
  this.type = type;
  this.description = description;
  this._defaultValue = defaultValue;

  this.onChange = util.createEvent('Setting.onChange');
  this.setDefault();
}




Setting.prototype.setDefault = function() {
  this.value = this._defaultValue;
};




Object.defineProperty(Setting.prototype, 'value', {
  get: function() {
    return settingValues[this.name];
  },

  set: function(value) {
    settingValues[this.name] = value;
    settingStorage.save(settingValues);
    this.onChange({ setting: this, value: value });
  },

  enumerable: true
});
