















'use strict';

var Promise = require('../util/promise').Promise;
var Status = require('./types').Status;
var Conversion = require('./types').Conversion;
var BlankArgument = require('./types').BlankArgument;
var SelectionType = require('./selection').SelectionType;

exports.items = [
  {
    
    item: 'type',
    name: 'boolean',
    parent: 'selection',

    getSpec: function() {
      return 'boolean';
    },

    lookup: [
      { name: 'false', value: false },
      { name: 'true', value: true }
    ],

    parse: function(arg, context) {
      if (arg.type === 'TrueNamedArgument') {
        return Promise.resolve(new Conversion(true, arg));
      }
      if (arg.type === 'FalseNamedArgument') {
        return Promise.resolve(new Conversion(false, arg));
      }
      return SelectionType.prototype.parse.call(this, arg, context);
    },

    stringify: function(value, context) {
      if (value == null) {
        return '';
      }
      return '' + value;
    },

    getBlank: function(context) {
      return new Conversion(false, new BlankArgument(), Status.VALID, '',
                            Promise.resolve(this.lookup));
    }
  }
];
