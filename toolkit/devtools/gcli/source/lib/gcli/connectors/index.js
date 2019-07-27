















'use strict';

var api = require('../api');
var Commands = require('../commands/commands').Commands;
var Types = require('../types/types').Types;


require('../util/legacy');














var items = [
  
  require('../types/delegate').items,
  require('../types/selection').items,
  require('../types/array').items,

  require('../types/boolean').items,
  require('../types/command').items,
  require('../types/date').items,
  require('../types/file').items,
  require('../types/javascript').items,
  require('../types/node').items,
  require('../types/number').items,
  require('../types/resource').items,
  require('../types/setting').items,
  require('../types/string').items,
  require('../types/union').items,
  require('../types/url').items,

  require('../fields/fields').items,
  require('../fields/delegate').items,
  require('../fields/selection').items,

  require('../ui/intro').items,
  require('../ui/focus').items,

  require('../converters/converters').items,
  require('../converters/basic').items,
  require('../converters/html').items,
  require('../converters/terminal').items,

  require('../languages/command').items,
  require('../languages/javascript').items,

  require('./direct').items,
  
  require('./websocket').items,
  require('./xhr').items,

  require('../commands/context').items,

].reduce(function(prev, curr) { return prev.concat(curr); }, []);





var requiredConverters = [
  require('../cli').items,

  require('../commands/clear').items,
  require('../commands/connect').items,
  require('../commands/exec').items,
  require('../commands/global').items,
  require('../commands/help').items,
  require('../commands/intro').items,
  require('../commands/lang').items,
  require('../commands/preflist').items,
  require('../commands/pref').items,
  require('../commands/test').items,

].reduce(function(prev, curr) { return prev.concat(curr); }, [])
 .filter(function(item) { return item.item === 'converter'; });





exports.connect = function(options) {
  options = options || {};

  var system = api.createSystem();

  
  exports.api = system;

  options.types = system.types = new Types();
  options.commands = system.commands = new Commands(system.types);

  system.addItems(items);
  system.addItems(requiredConverters);

  var connector = system.connectors.get(options.connector);
  return connector.connect(options.url).then(function(connection) {
    options.connection = connection;
    connection.on('commandsChanged', function(specs) {
      exports.addItems(system, specs, connection);
    });

    return connection.call('specs').then(function(specs) {
      exports.addItems(system, specs, connection);
      return connection;
    });
  });
};

exports.addItems = function(gcli, specs, connection) {
  exports.removeRemoteItems(gcli, connection);
  var remoteItems = exports.addLocalFunctions(specs, connection);
  gcli.addItems(remoteItems);
};





exports.addLocalFunctions = function(specs, connection) {
  
  
  specs.forEach(function(commandSpec) {
    
    commandSpec.connection = connection;
    commandSpec.params.forEach(function(param) {
      param.type.connection = connection;
    });

    if (!commandSpec.isParent) {
      commandSpec.exec = function(args, context) {
        var data = {
          typed: (context.prefix ? context.prefix + ' ' : '') + context.typed
        };

        return connection.call('execute', data).then(function(reply) {
          var typedData = context.typedData(reply.type, reply.data);
          if (!reply.error) {
            return typedData;
          }
          else {
            throw typedData;
          }
        });
      };
    }

    commandSpec.isProxy = true;
  });

  return specs;
};

exports.removeRemoteItems = function(gcli, connection) {
  gcli.commands.getAll().forEach(function(command) {
    if (command.connection === connection) {
      gcli.commands.remove(command);
    }
  });
};
