















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




var types;





function Setting(prefSpec) {
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




Object.defineProperty(Setting.prototype, 'type', {
  get: function() {
    switch (imports.prefBranch.getPrefType(this.name)) {
      case imports.prefBranch.PREF_BOOL:
        return types.createType('boolean');

      case imports.prefBranch.PREF_INT:
        return types.createType('number');

      case imports.prefBranch.PREF_STRING:
        return types.createType('string');

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




Setting.prototype.setDefault = function() {
  imports.prefBranch.clearUserPref(this.name);
  Services.prefs.savePrefFile(null);
};





var settingsAll = [];




var settingsMap = new Map();




var hasReadSystem = false;




function reset() {
  settingsMap = new Map();
  settingsAll = [];
  hasReadSystem = false;
}




exports.startup = function(t) {
  reset();
  types = t;
  if (types == null) {
    throw new Error('no types');
  }
};

exports.shutdown = function() {
  reset();
};





function readSystem() {
  if (hasReadSystem) {
    return;
  }

  imports.prefBranch.getChildList('').forEach(function(name) {
    var setting = new Setting(name);
    settingsAll.push(setting);
    settingsMap.set(name, setting);
  });

  settingsAll.sort(function(s1, s2) {
    return s1.name.localeCompare(s2.name);
  });

  hasReadSystem = true;
}





exports.getAll = function(filter) {
  readSystem();

  if (filter == null) {
    return settingsAll;
  }

  return settingsAll.filter(function(setting) {
    return setting.name.indexOf(filter) !== -1;
  });
};




exports.addSetting = function(prefSpec) {
  var setting = new Setting(prefSpec);

  if (settingsMap.has(setting.name)) {
    
    for (var i = 0; i < settingsAll.length; i++) {
      if (settingsAll[i].name === setting.name) {
        settingsAll[i] = setting;
      }
    }
  }

  settingsMap.set(setting.name, setting);
  exports.onChange({ added: setting.name });

  return setting;
};










exports.getSetting = function(name) {
  
  
  var found = settingsMap.get(name);
  if (!found) {
    found = settingsMap.get(DEVTOOLS_PREFIX + name);
  }

  if (found) {
    return found;
  }

  if (hasReadSystem) {
    return undefined;
  }
  else {
    readSystem();
    found = settingsMap.get(name);
    if (!found) {
      found = settingsMap.get(DEVTOOLS_PREFIX + name);
    }
    return found;
  }
};




exports.onChange = util.createEvent('Settings.onChange');




exports.removeSetting = function() { };
