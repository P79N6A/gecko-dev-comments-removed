















'use strict';

var promise = require('../util/promise');
var ArrayConversion = require('./types').ArrayConversion;
var ArrayArgument = require('./types').ArrayArgument;

exports.items = [
  {
    
    item: 'type',
    name: 'array',
    subtype: undefined,

    constructor: function() {
      if (!this.subtype) {
        console.error('Array.typeSpec is missing subtype. Assuming string.' +
            this.name);
        this.subtype = 'string';
      }
      this.subtype = this.types.createType(this.subtype);
    },

    getSpec: function(commandName, paramName) {
      return {
        name: 'array',
        subtype: this.subtype.getSpec(commandName, paramName),
      };
    },

    stringify: function(values, context) {
      if (values == null) {
        return '';
      }
      
      return values.join(' ');
    },

    parse: function(arg, context) {
      if (arg.type !== 'ArrayArgument') {
        console.error('non ArrayArgument to ArrayType.parse', arg);
        throw new Error('non ArrayArgument to ArrayType.parse');
      }

      
      
      
      
      var subArgParse = function(subArg) {
        return this.subtype.parse(subArg, context).then(function(conversion) {
          subArg.conversion = conversion;
          return conversion;
        }.bind(this));
      }.bind(this);

      var conversionPromises = arg.getArguments().map(subArgParse);
      return promise.all(conversionPromises).then(function(conversions) {
        return new ArrayConversion(conversions, arg);
      });
    },

    getBlank: function(context) {
      return new ArrayConversion([], new ArrayArgument());
    }
  },
];
