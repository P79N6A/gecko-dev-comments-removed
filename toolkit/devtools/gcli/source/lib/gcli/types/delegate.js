















'use strict';

var Promise = require('../util/promise').Promise;
var Conversion = require('./types').Conversion;
var Status = require('./types').Status;




exports.items = [
  
  {
    item: 'type',
    name: 'delegate',

    constructor: function() {
      if (typeof this.delegateType !== 'function' &&
          typeof this.delegateType !== 'string') {
        throw new Error('Instances of DelegateType need typeSpec.delegateType' +
                        ' to be a function that returns a type');
      }
    },

    getSpec: function(commandName, paramName) {
      return {
        name: 'delegate',
        param: paramName
      };
    },

    
    
    
    delegateType: function(context) {
      throw new Error('Not implemented');
    },

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

    decrement: function(value, context) {
      return this.getType(context).then(function(delegated) {
        return delegated.decrement ?
               delegated.decrement(value, context) :
               undefined;
      }.bind(this));
    },

    increment: function(value, context) {
      return this.getType(context).then(function(delegated) {
        return delegated.increment ?
               delegated.increment(value, context) :
               undefined;
      }.bind(this));
    },

    getType: function(context) {
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
    param: undefined,

    stringify: function(value, context) {
      
      
      
      
      
      if (typeof value.stringified === 'string') {
        return value.stringified;
      }
      throw new Error('Can\'t stringify that value');
    },

    parse: function(arg, context) {
      var args = { typed: context.typed, param: this.param };
      return this.connection.call('typeparse', args).then(function(json) {
        var status = Status.fromString(json.status);
        var val = { stringified: arg.text };
        return new Conversion(val, arg, status, json.message, json.predictions);
      });
    },

    decrement: function(value, context) {
      var args = { typed: context.typed, param: this.param };
      return this.connection.call('typedecrement', args).then(function(json) {
        return { stringified: json.arg };
      });
    },

    increment: function(value, context) {
      var args = { typed: context.typed, param: this.param };
      return this.connection.call('typeincrement', args).then(function(json) {
        return { stringified: json.arg };
      });
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
