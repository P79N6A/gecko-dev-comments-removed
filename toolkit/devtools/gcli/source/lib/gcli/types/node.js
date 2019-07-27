















'use strict';

var Promise = require('../util/promise').Promise;
var Highlighter = require('../util/host').Highlighter;
var l10n = require('../util/l10n');
var util = require('../util/util');
var Status = require('./types').Status;
var Conversion = require('./types').Conversion;
var BlankArgument = require('./types').BlankArgument;





function onEnter(assignment) {
  
  
  assignment.highlighter = new Highlighter(context.environment.window.document);
  assignment.highlighter.nodelist = assignment.conversion.matches;
}


function onLeave(assignment) {
  if (!assignment.highlighter) {
    return;
  }

  assignment.highlighter.destroy();
  delete assignment.highlighter;
}

function onChange(assignment) {
  if (assignment.conversion.matches == null) {
    return;
  }
  if (!assignment.highlighter) {
    return;
  }

  assignment.highlighter.nodelist = assignment.conversion.matches;
}




exports.items = [
  {
    
    item: 'type',
    name: 'node',

    getSpec: function(commandName, paramName) {
      return {
        name: 'remote',
        commandName: commandName,
        paramName: paramName
      };
    },

    stringify: function(value, context) {
      if (value == null) {
        return '';
      }
      return value.__gcliQuery || 'Error';
    },

    parse: function(arg, context) {
      var reply;

      if (arg.text === '') {
        reply = new Conversion(undefined, arg, Status.INCOMPLETE);
      }
      else {
        var nodes;
        try {
          nodes = context.environment.window.document.querySelectorAll(arg.text);
          if (nodes.length === 0) {
            reply = new Conversion(undefined, arg, Status.INCOMPLETE,
                                   l10n.lookup('nodeParseNone'));
          }
          else if (nodes.length === 1) {
            var node = nodes.item(0);
            node.__gcliQuery = arg.text;

            reply = new Conversion(node, arg, Status.VALID, '');
          }
          else {
            var msg = l10n.lookupFormat('nodeParseMultiple', [ nodes.length ]);
            reply = new Conversion(undefined, arg, Status.ERROR, msg);
          }

          reply.matches = nodes;
        }
        catch (ex) {
          reply = new Conversion(undefined, arg, Status.ERROR,
                                 l10n.lookup('nodeParseSyntax'));
        }
      }

      return Promise.resolve(reply);
    },

    
    
    
  },
  {
    
    item: 'type',
    name: 'nodelist',

    
    
    
    
    
    
    
    
    allowEmpty: false,

    constructor: function() {
      if (typeof this.allowEmpty !== 'boolean') {
        throw new Error('Legal values for allowEmpty are [true|false]');
      }
    },

    getSpec: function(commandName, paramName) {
      return {
        name: 'remote',
        commandName: commandName,
        paramName: paramName,
        blankIsValid: true
      };
    },

    getBlank: function(context) {
      var emptyNodeList = [];
      if (context != null && context.environment.window != null) {
        var doc = context.environment.window.document;
        emptyNodeList = util.createEmptyNodeList(doc);
      }
      return new Conversion(emptyNodeList, new BlankArgument(), Status.VALID);
    },

    stringify: function(value, context) {
      if (value == null) {
        return '';
      }
      return value.__gcliQuery || 'Error';
    },

    parse: function(arg, context) {
      var reply;
      try {
        if (arg.text === '') {
          reply = new Conversion(undefined, arg, Status.INCOMPLETE);
        }
        else {
          var nodes = context.environment.window.document.querySelectorAll(arg.text);

          if (nodes.length === 0 && !this.allowEmpty) {
            reply = new Conversion(undefined, arg, Status.INCOMPLETE,
                                   l10n.lookup('nodeParseNone'));
          }
          else {
            nodes.__gcliQuery = arg.text;
            reply = new Conversion(nodes, arg, Status.VALID, '');
          }

          reply.matches = nodes;
        }
      }
      catch (ex) {
        reply = new Conversion(undefined, arg, Status.ERROR,
                               l10n.lookup('nodeParseSyntax'));
      }

      return Promise.resolve(reply);
    },

    
    
    
  }
];
