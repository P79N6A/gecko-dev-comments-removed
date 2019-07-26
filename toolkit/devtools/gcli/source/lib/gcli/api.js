















'use strict';

var centralCanon = require('./commands/commands').centralCanon;
var connectors = require('./connectors/connectors');
var converters = require('./converters/converters');
var fields = require('./fields/fields');
var languages = require('./languages/languages');
var settings = require('./settings');
var centralTypes = require('./types/types').centralTypes;




exports.getApi = function() {
  var canon = centralCanon;
  var types = centralTypes;

  settings.startup(types);

  var api = {
    addCommand: function(item) { return canon.addCommand(item); },
    removeCommand: function(item) { return canon.removeCommand(item); },
    addConnector: connectors.addConnector,
    removeConnector: connectors.removeConnector,
    addConverter: converters.addConverter,
    removeConverter: converters.removeConverter,
    addLanguage: languages.addLanguage,
    removeLanguage: languages.removeLanguage,
    addType: function(item) { return types.addType(item); },
    removeType: function(item) { return types.removeType(item); },

    addItems: function(items) {
      items.forEach(function(item) {
        
        
        var type = item.item;
        if (type == null && item.prototype) {
          type = item.prototype.item;
        }
        if (type === 'command') {
          canon.addCommand(item);
        }
        else if (type === 'connector') {
          connectors.addConnector(item);
        }
        else if (type === 'converter') {
          converters.addConverter(item);
        }
        else if (type === 'field') {
          fields.addField(item);
        }
        else if (type === 'language') {
          languages.addLanguage(item);
        }
        else if (type === 'setting') {
          settings.addSetting(item);
        }
        else if (type === 'type') {
          types.addType(item);
        }
        else {
          console.error('Error for: ', item);
          throw new Error('item property not found');
        }
      });
    },

    removeItems: function(items) {
      items.forEach(function(item) {
        if (item.item === 'command') {
          canon.removeCommand(item);
        }
        else if (item.item === 'connector') {
          connectors.removeConnector(item);
        }
        else if (item.item === 'converter') {
          converters.removeConverter(item);
        }
        else if (item.item === 'field') {
          fields.removeField(item);
        }
        else if (item.item === 'language') {
          languages.removeLanguage(item);
        }
        else if (item.item === 'settings') {
          settings.removeSetting(types, item);
        }
        else if (item.item === 'type') {
          types.removeType(item);
        }
        else {
          throw new Error('item property not found');
        }
      });
    }
  };

  Object.defineProperty(api, 'canon', {
    get: function() { return canon; },
    set: function(c) { canon = c; },
    enumerable: true
  });

  Object.defineProperty(api, 'types', {
    get: function() { return types; },
    set: function(t) {
      types = t;
      settings.startup(types);
    },
    enumerable: true
  });

  return api;
};





exports.populateApi = function(obj) {
  var exportable = exports.getApi();
  Object.keys(exportable).forEach(function(key) {
    obj[key] = exportable[key];
  });
};
