















'use strict';

var imports = {};

var Cc = require('chrome').Cc;
var Ci = require('chrome').Ci;
var Cu = require('chrome').Cu;

var XPCOMUtils = Cu.import('resource://gre/modules/XPCOMUtils.jsm', {}).XPCOMUtils;
var Services = Cu.import('resource://gre/modules/Services.jsm', {}).Services;

XPCOMUtils.defineLazyGetter(imports, 'prefBranch', function() {
  var prefService = Cc['@mozilla.org/preferences-service;1']
          .getService(Ci.nsIPrefService);
  return prefService.getBranch(null).QueryInterface(Ci.nsIPrefBranch2);
});

XPCOMUtils.defineLazyGetter(imports, 'supportsString', function() {
  return Cc['@mozilla.org/supports-string;1']
          .createInstance(Ci.nsISupportsString);
});

var util = require('./util/util');




var DEVTOOLS_PREFIX = 'devtools.gcli.';




function Settings(types, settingValues) {
  this._types = types;

  if (settingValues != null) {
    throw new Error('settingValues is not supported when writing to prefs');
  }

  
  this._settingsAll = [];

  
  this._settingsMap = new Map();

  
  this._hasReadSystem = false;

  
  this.onChange = util.createEvent('Settings.onChange');
}





Settings.prototype._readSystem = function() {
  if (this._hasReadSystem) {
    return;
  }

  imports.prefBranch.getChildList('').forEach(function(name) {
    var setting = new Setting(this, name);
    this._settingsAll.push(setting);
    this._settingsMap.set(name, setting);
  }.bind(this));

  this._settingsAll.sort(function(s1, s2) {
    return s1.name.localeCompare(s2.name);
  }.bind(this));

  this._hasReadSystem = true;
};





Settings.prototype.getAll = function(filter) {
  this._readSystem();

  if (filter == null) {
    return this._settingsAll;
  }

  return this._settingsAll.filter(function(setting) {
    return setting.name.indexOf(filter) !== -1;
  }.bind(this));
};





Settings.prototype.add = function(prefSpec) {
  var setting = new Setting(this, prefSpec);

  if (this._settingsMap.has(setting.name)) {
    
    for (var i = 0; i < this._settingsAll.length; i++) {
      if (this._settingsAll[i].name === setting.name) {
        this._settingsAll[i] = setting;
      }
    }
  }

  this._settingsMap.set(setting.name, setting);
  this.onChange({ added: setting.name });

  return setting;
};










Settings.prototype.get = function(name) {
  
  
  var found = this._settingsMap.get(name);
  if (!found) {
    found = this._settingsMap.get(DEVTOOLS_PREFIX + name);
  }

  if (found) {
    return found;
  }

  if (this._hasReadSystem) {
    return undefined;
  }
  else {
    this._readSystem();
    found = this._settingsMap.get(name);
    if (!found) {
      found = this._settingsMap.get(DEVTOOLS_PREFIX + name);
    }
    return found;
  }
};




Settings.prototype.remove = function() {
};

exports.Settings = Settings;





function Setting(settings, prefSpec) {
  this._settings = settings;
  if (typeof prefSpec === 'string') {
    
    this.name = prefSpec;
    this.description = '';
  }
  else {
    
    this.name = DEVTOOLS_PREFIX + prefSpec.name;

    if (prefSpec.ignoreTypeDifference !== true && prefSpec.type) {
      if (this.type.name !== prefSpec.type) {
        throw new Error('Locally declared type (' + prefSpec.type + ') != ' +
            'Mozilla declared type (' + this.type.name + ') for ' + this.name);
      }
    }

    this.description = prefSpec.description;
  }

  this.onChange = util.createEvent('Setting.onChange');
}




Setting.prototype.setDefault = function() {
  imports.prefBranch.clearUserPref(this.name);
  Services.prefs.savePrefFile(null);
};




Object.defineProperty(Setting.prototype, 'type', {
  get: function() {
    switch (imports.prefBranch.getPrefType(this.name)) {
      case imports.prefBranch.PREF_BOOL:
        return this._settings._types.createType('boolean');

      case imports.prefBranch.PREF_INT:
        return this._settings._types.createType('number');

      case imports.prefBranch.PREF_STRING:
        return this._settings._types.createType('string');

      default:
        throw new Error('Unknown type for ' + this.name);
    }
  },
  enumerable: true
});




Object.defineProperty(Setting.prototype, 'value', {
  get: function() {
    switch (imports.prefBranch.getPrefType(this.name)) {
      case imports.prefBranch.PREF_BOOL:
        return imports.prefBranch.getBoolPref(this.name);

      case imports.prefBranch.PREF_INT:
        return imports.prefBranch.getIntPref(this.name);

      case imports.prefBranch.PREF_STRING:
        var value = imports.prefBranch.getComplexValue(this.name,
                Ci.nsISupportsString).data;
        
        if (/^chrome:\/\/.+\/locale\/.+\.properties/.test(value)) {
          value = imports.prefBranch.getComplexValue(this.name,
                  Ci.nsIPrefLocalizedString).data;
        }
        return value;

      default:
        throw new Error('Invalid value for ' + this.name);
    }
  },

  set: function(value) {
    if (imports.prefBranch.prefIsLocked(this.name)) {
      throw new Error('Locked preference ' + this.name);
    }

    switch (imports.prefBranch.getPrefType(this.name)) {
      case imports.prefBranch.PREF_BOOL:
        imports.prefBranch.setBoolPref(this.name, value);
        break;

      case imports.prefBranch.PREF_INT:
        imports.prefBranch.setIntPref(this.name, value);
        break;

      case imports.prefBranch.PREF_STRING:
        imports.supportsString.data = value;
        imports.prefBranch.setComplexValue(this.name,
                Ci.nsISupportsString,
                imports.supportsString);
        break;

      default:
        throw new Error('Invalid value for ' + this.name);
    }

    Services.prefs.savePrefFile(null);
  },

  enumerable: true
});
