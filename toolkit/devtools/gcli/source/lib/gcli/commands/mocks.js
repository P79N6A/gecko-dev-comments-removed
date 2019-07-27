















'use strict';

var cli = require('../cli');
var mockCommands = require('../test/mockCommands');
var mockSettings = require('../test/mockSettings');

exports.items = [
  {
    item: 'command',
    name: 'mocks',
    description: 'Add/remove mock commands',
    params: [
      {
        name: 'included',
        type: {
          name: 'selection',
          data: [ 'on', 'off' ]
        },
        description: 'Turn mock commands on or off',
      }
    ],
    returnType: 'string',

    exec: function(args, context) {
      var requisition = cli.getMapping(context).requisition;
      this[args.included](requisition);
      return 'Mock commands are now ' + args.included;
    },

    on: function(requisition) {
      mockCommands.setup(requisition);
      mockSettings.setup(requisition.system);
    },

    off: function(requisition) {
      mockCommands.shutdown(requisition);
      mockSettings.shutdown(requisition.system);
    }
  }
];
