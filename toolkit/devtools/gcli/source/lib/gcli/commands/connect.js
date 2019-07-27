















'use strict';

var l10n = require('../util/l10n');
var cli = require('../cli');
var GcliFront = require('../connectors/remoted').GcliFront;




var fronts = {};




var connection = {
  item: 'type',
  name: 'connection',
  parent: 'selection',
  lookup: function() {
    return Object.keys(fronts).map(function(prefix) {
      return { name: prefix, value: fronts[prefix] };
    });
  }
};




var connector = {
  item: 'type',
  name: 'connector',
  parent: 'selection',
  lookup: function(context) {
    var connectors = context.system.connectors;
    return connectors.getAll().map(function(connector) {
      return { name: connector.name, value: connector };
    });
  }
};




var connect = {
  item: 'command',
  name: 'connect',
  description: l10n.lookup('connectDesc'),
  manual: l10n.lookup('connectManual'),
  params: [
    {
      name: 'prefix',
      type: 'string',
      description: l10n.lookup('connectPrefixDesc')
    },
    {
      name: 'method',
      short: 'm',
      type: 'connector',
      description: l10n.lookup('connectMethodDesc'),
      defaultValue: null,
      option: true
    },
    {
      name: 'url',
      short: 'u',
      type: 'string',
      description: l10n.lookup('connectUrlDesc'),
      defaultValue: null,
      option: true
    }
  ],
  returnType: 'string',

  exec: function(args, context) {
    if (fronts[args.prefix] != null) {
      throw new Error(l10n.lookupFormat('connectDupReply', [ args.prefix ]));
    }

    args.method = args.method || context.system.connectors.get('xhr');

    return GcliFront.create(args.method, args.url).then(function(front) {
      
      front.prefix = args.prefix;
      fronts[args.prefix] = front;

      return front.specs().then(function(specs) {
        var remoter = this.createRemoter(args.prefix, front);
        var commands = cli.getMapping(context).requisition.system.commands;
        commands.addProxyCommands(specs, remoter, args.prefix, args.url);

        

        
        return l10n.lookupFormat('connectReply',
                                 [ Object.keys(specs).length + 1 ]);
      }.bind(this));
    }.bind(this));
  },

  



  createRemoter: function(prefix, front) {
    return function(cmdArgs, context) {
      var typed = context.typed;

      
      
      if (typed.indexOf(prefix) === 0) {
        typed = typed.substring(prefix.length).replace(/^ */, '');
      }

      return front.execute(typed).then(function(reply) {
        var typedData = context.typedData(reply.type, reply.data);
        if (!reply.error) {
          return typedData;
        }
        else {
          throw typedData;
        }
      });
    }.bind(this);
  }
};




var disconnect = {
  item: 'command',
  name: 'disconnect',
  description: l10n.lookup('disconnectDesc2'),
  manual: l10n.lookup('disconnectManual2'),
  params: [
    {
      name: 'prefix',
      type: 'connection',
      description: l10n.lookup('disconnectPrefixDesc')
    }
  ],
  returnType: 'string',

  exec: function(args, context) {
    var front = args.prefix;
    return front.connection.disconnect().then(function() {
      var commands = cli.getMapping(context).requisition.system.commands;
      var removed = commands.removeProxyCommands(front.prefix);
      delete fronts[front.prefix];
      return l10n.lookupFormat('disconnectReply', [ removed.length ]);
    });
  }
};

exports.items = [ connection, connector, connect, disconnect ];
