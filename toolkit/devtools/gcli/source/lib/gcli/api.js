















'use strict';

var Promise = require('./util/promise').Promise;
var Commands = require('./commands/commands').Commands;
var Connectors = require('./connectors/connectors').Connectors;
var Converters = require('./converters/converters').Converters;
var Fields = require('./fields/fields').Fields;
var Languages = require('./languages/languages').Languages;
var Settings = require('./settings').Settings;
var Types = require('./types/types').Types;




exports.createSystem = function() {

  var components = {
    connector: new Connectors(),
    converter: new Converters(),
    field: new Fields(),
    language: new Languages(),
    type: new Types()
  };
  components.setting = new Settings(components.type);
  components.command = new Commands(components.type);

  var getItemType = function(item) {
    if (item.item) {
      return item.item;
    }
    
    
    return (item.prototype && item.prototype.item) ?
           item.prototype.item : 'command';
  };

  var addItem = function(item) {
    components[getItemType(item)].add(item);
  };

  var removeItem = function(item) {
    components[getItemType(item)].remove(item);
  };

  





  var loadableModules = {};

  



  var loadedModules = {};

  var unloadModule = function(name) {
    var existingModule = loadedModules[name];
    if (existingModule != null) {
      existingModule.items.forEach(removeItem);
    }
    delete loadedModules[name];
  };

  var loadModule = function(name) {
    var existingModule = loadedModules[name];
    unloadModule(name);

    
    try {
      var loader = loadableModules[name];
      return Promise.resolve(loader(name)).then(function(newModule) {
        if (existingModule === newModule) {
          return;
        }

        if (newModule == null) {
          throw 'Module \'' + name + '\' not found';
        }

        if (newModule.items == null || typeof newModule.items.forEach !== 'function') {
          console.log('Exported properties: ' + Object.keys(newModule).join(', '));
          throw 'Module \'' + name + '\' has no \'items\' array export';
        }

        newModule.items.forEach(addItem);

        loadedModules[name] = newModule;
      });
    }
    catch (ex) {
      console.error(ex);
      return Promise.reject('Failure when loading \'' + name + '\'');
    }
  };

  var pendingChanges = false;

  var api = {
    addItems: function(items) {
      items.forEach(addItem);
    },

    removeItems: function(items) {
      items.forEach(removeItem);
    },

    addItemsByModule: function(names, options) {
      options = options || {};
      if (typeof names === 'string') {
        names = [ names ];
      }
      names.forEach(function(name) {
        if (options.loader == null) {
          options.loader = function(name) {
            return require(name);
          };
        }
        loadableModules[name] = options.loader;

        if (options.delayedLoad) {
          pendingChanges = true;
        }
        else {
          loadModule(name).then(null, console.error);
        }
      });
    },

    removeItemsByModule: function(name) {
      delete loadableModules[name];
      unloadModule(name);
    },

    load: function() {
      if (!pendingChanges) {
        return Promise.resolve();
      }

      
      var modules = Object.keys(loadedModules).map(function(name) {
        return loadedModules[name];
      });

      var promises = Object.keys(loadableModules).map(function(name) {
        delete modules[name];
        return loadModule(name);
      });

      Object.keys(modules).forEach(unloadModule);
      pendingChanges = false;

      return Promise.all(promises);
    }
  };

  Object.defineProperty(api, 'commands', {
    get: function() { return components.command; },
    set: function(commands) { components.command = commands; },
    enumerable: true
  });

  Object.defineProperty(api, 'connectors', {
    get: function() { return components.connector; },
    enumerable: true
  });

  Object.defineProperty(api, 'converters', {
    get: function() { return components.converter; },
    enumerable: true
  });

  Object.defineProperty(api, 'fields', {
    get: function() { return components.field; },
    enumerable: true
  });

  Object.defineProperty(api, 'languages', {
    get: function() { return components.language; },
    enumerable: true
  });

  Object.defineProperty(api, 'settings', {
    get: function() { return components.setting; },
    enumerable: true
  });

  Object.defineProperty(api, 'types', {
    get: function() { return components.type; },
    set: function(types) {
      components.type = types;
      components.command.types = types;
      components.setting.types = types;
    },
    enumerable: true
  });

  return api;
};
