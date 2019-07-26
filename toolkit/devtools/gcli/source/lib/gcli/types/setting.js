















'use strict';

var settings = require('../settings');

exports.items = [
  {
    
    item: 'type',
    name: 'setting',
    parent: 'selection',
    cacheable: true,
    constructor: function() {
      settings.onChange.add(function(ev) {
        this.clearCache();
      }, this);
    },
    lookup: function() {
      return settings.getAll().map(function(setting) {
        return { name: setting.name, value: setting };
      });
    }
  },
  {
    
    
    
    
    item: 'type',
    name: 'settingValue',
    parent: 'delegate',
    settingParamName: 'setting',
    delegateType: function(context) {
      if (context != null) {
        var setting = context.getArgsObject()[this.settingParamName];
        if (setting != null) {
          return setting.type;
        }
      }

      return 'blank';
    }
  }
];
