















'use strict';

var Promise = require('./util/promise').Promise;
var util = require('./util/util');
var Commands = require('./commands/commands').Commands;
var Connectors = require('./connectors/connectors').Connectors;
var Converters = require('./converters/converters').Converters;
var Fields = require('./fields/fields').Fields;
var Languages = require('./languages/languages').Languages;
var Settings = require('./settings').Settings;
var Types = require('./types/types').Types;













exports.createSystem = function(options) {
  options = options || {};
  var location = options.location;

  
  
  
  var components = {
    connector: options.connectors || new Connectors(),
    converter: options.converters || new Converters(),
    field: options.fields || new Fields(),
    language: options.languages || new Languages(),
    type: options.types || new Types()
  };
  components.setting = new Settings(components.type);
  components.command = new Commands(components.type, location);

  var getItemType = function(item) {
    if (item.item) {
      return item.item;
    }
    
    
    return (item.prototype && item.prototype.item) ?
           item.prototype.item : 'command';
  };

  var addItem = function(item) {
    try {
      components[getItemType(item)].add(item);
    }
    catch (ex) {
      if (item != null) {
        console.error('While adding: ' + item.name);
      }
      throw ex;
    }
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
      console.error('Failed to load module ' + name + ': ' + ex);
      console.error(ex.stack);

      return Promise.resolve();
    }
  };

  var pendingChanges = false;

  var system = {
    addItems: function(items) {
      items.forEach(addItem);
    },

    removeItems: function(items) {
      items.forEach(removeItem);
    },

    addItemsByModule: function(names, options) {
      var promises = [];

      options = options || {};
      if (!options.delayedLoad) {
        
        this.commands.onCommandsChange.holdFire();
      }

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
          promises.push(loadModule(name).catch(console.error));
        }
      });

      if (options.delayedLoad) {
        return Promise.resolve();
      }
      else {
        return Promise.all(promises).then(function() {
          this.commands.onCommandsChange.resumeFire();
        }.bind(this));
      }
    },

    removeItemsByModule: function(name) {
      this.commands.onCommandsChange.holdFire();

      delete loadableModules[name];
      unloadModule(name);

      this.commands.onCommandsChange.resumeFire();
    },

    load: function() {
      if (!pendingChanges) {
        return Promise.resolve();
      }
      this.commands.onCommandsChange.holdFire();

      
      var modules = Object.keys(loadedModules).map(function(name) {
        return loadedModules[name];
      });

      var promises = Object.keys(loadableModules).map(function(name) {
        delete modules[name];
        return loadModule(name).catch(console.error);
      });

      Object.keys(modules).forEach(unloadModule);
      pendingChanges = false;

      return Promise.all(promises).then(function() {
        this.commands.onCommandsChange.resumeFire();
      }.bind(this));
    },

    destroy: function() {
      this.commands.onCommandsChange.holdFire();

      Object.keys(loadedModules).forEach(function(name) {
        unloadModule(name);
      });

      this.commands.onCommandsChange.resumeFire();
    },

    toString: function() {
      return 'System [' +
             'commands:' + components.command.getAll().length + ', ' +
             'connectors:' + components.connector.getAll().length + ', ' +
             'converters:' + components.converter.getAll().length + ', ' +
             'fields:' + components.field.getAll().length + ', ' +
             'settings:' + components.setting.getAll().length + ', ' +
             'types:' + components.type.getTypeNames().length + ']';
    }
  };

  Object.defineProperty(system, 'commands', {
    get: function() { return components.command; },
    enumerable: true
  });

  Object.defineProperty(system, 'connectors', {
    get: function() { return components.connector; },
    enumerable: true
  });

  Object.defineProperty(system, 'converters', {
    get: function() { return components.converter; },
    enumerable: true
  });

  Object.defineProperty(system, 'fields', {
    get: function() { return components.field; },
    enumerable: true
  });

  Object.defineProperty(system, 'languages', {
    get: function() { return components.language; },
    enumerable: true
  });

  Object.defineProperty(system, 'settings', {
    get: function() { return components.setting; },
    enumerable: true
  });

  Object.defineProperty(system, 'types', {
    get: function() { return components.type; },
    enumerable: true
  });

  return system;
};










exports.connectFront = function(system, front, customProps) {
  system._handleCommandsChanged = function() {
    syncItems(system, front, customProps).catch(util.errorHandler);
  };
  front.on('commands-changed', system._handleCommandsChanged);

  return syncItems(system, front, customProps);
};




exports.disconnectFront = function(system, front) {
  front.off('commands-changed', system._handleCommandsChanged);
  system._handleCommandsChanged = undefined;
  removeItemsFromFront(system, front);
};





function syncItems(system, front, customProps) {
  return front.specs(customProps).then(function(specs) {
    removeItemsFromFront(system, front);

    var remoteItems = addLocalFunctions(specs, front);
    system.addItems(remoteItems);

    return system;
  });
};





function addLocalFunctions(specs, front) {
  
  
  specs.forEach(function(commandSpec) {
    
    
    commandSpec.front = front;

    
    
    
    
    commandSpec.params.forEach(function(param) {
      if (typeof param.type !== 'string') {
        param.type.front = front;
      }
    });

    if (!commandSpec.isParent) {
      commandSpec.exec = function(args, context) {
        var typed = (context.prefix ? context.prefix + ' ' : '') + context.typed;
        return front.execute(typed).then(function(reply) {
          var typedData = context.typedData(reply.type, reply.data);
          return reply.isError ? Promise.reject(typedData) : typedData;
        });
      };
    }

    commandSpec.isProxy = true;
  });

  return specs;
}





function removeItemsFromFront(system, front) {
  system.commands.getAll().forEach(function(command) {
    if (command.front === front) {
      system.commands.remove(command);
    }
  });
}
