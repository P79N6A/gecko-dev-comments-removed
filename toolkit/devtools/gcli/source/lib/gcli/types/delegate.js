















'use strict';

var Promise = require('../util/promise').Promise;
var Conversion = require('./types').Conversion;
var Status = require('./types').Status;
var BlankArgument = require('./types').BlankArgument;




exports.items = [
  
  {
    item: 'type',
    name: 'delegate',

    getSpec: function(commandName, paramName) {
      return {
        name: 'delegate',
        param: paramName
      };
    },

    
    
    
    delegateType: undefined,

    stringify: function(value, context) {
      return this.getType(context).then(function(delegated) {
        return delegated.stringify(value, context);
      }.bind(this));
    },

    parse: function(arg, context) {
      return this.getType(context).then(function(delegated) {
        return delegated.parse(arg, context);
      }.bind(this));
    },

    nudge: function(value, by, context) {
      return this.getType(context).then(function(delegated) {
        return delegated.nudge ?
               delegated.nudge(value, by, context) :
               undefined;
      }.bind(this));
    },

    getType: function(context) {
      if (this.delegateType === undefined) {
        return Promise.resolve(this.types.createType('blank'));
      }

      var type = this.delegateType(context);
      if (typeof type.parse !== 'function') {
        type = this.types.createType(type);
      }
      return Promise.resolve(type);
    },

    
    
    isDelegate: true,

    
    
    
    isImportant: false
  },
  {
    item: 'type',
    name: 'remote',
    paramName: undefined,
    blankIsValid: false,

    getSpec: function(commandName, paramName) {
      return {
        name: 'remote',
        commandName: commandName,
        paramName: paramName,
        blankIsValid: this.blankIsValid
      };
    },

    getBlank: function(context) {
      if (this.blankIsValid) {
        return new Conversion({ stringified: '' },
                              new BlankArgument(), Status.VALID);
      }
      else {
        return new Conversion(undefined, new BlankArgument(),
                              Status.INCOMPLETE, '');
      }
    },

    stringify: function(value, context) {
      if (value == null) {
        return '';
      }
      
      
      
      
      
      if (typeof value.stringified === 'string') {
        return value.stringified;
      }
      throw new Error('Can\'t stringify that value');
    },

    parse: function(arg, context) {
      return this.front.parseType(context.typed, this.paramName).then(function(json) {
        var status = Status.fromString(json.status);
        return new Conversion(undefined, arg, status, json.message, json.predictions);
      }.bind(this));
    },

    nudge: function(value, by, context) {
      return this.front.nudgeType(context.typed, by, this.paramName).then(function(json) {
        return { stringified: json.arg };
      }.bind(this));
    }
  },
  
  
  {
    item: 'type',
    name: 'blank',

    getSpec: function(commandName, paramName) {
      return 'blank';
    },

    stringify: function(value, context) {
      return '';
    },

    parse: function(arg, context) {
      return Promise.resolve(new Conversion(undefined, arg));
    }
  }
];
