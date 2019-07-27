















'use strict';






















var fileparser = require('./fileparser');
var Conversion = require('./types').Conversion;

exports.items = [
  {
    item: 'type',
    name: 'file',

    filetype: 'any',    
    existing: 'maybe',  
    matches: undefined, 

    hasPredictions: true,

    constructor: function() {
      if (this.filetype !== 'any' && this.filetype !== 'file' &&
          this.filetype !== 'directory') {
        throw new Error('filetype must be one of [any|file|directory]');
      }

      if (this.existing !== 'yes' && this.existing !== 'no' &&
          this.existing !== 'maybe') {
        throw new Error('existing must be one of [yes|no|maybe]');
      }
    },

    getSpec: function() {
      var matches = (typeof this.matches === 'string' || this.matches == null) ?
                    this.matches :
                    this.matches.source; 
      return {
        name: 'file',
        filetype: this.filetype,
        existing: this.existing,
        matches: matches
      };
    },

    stringify: function(file) {
      if (file == null) {
        return '';
      }

      return file.toString();
    },

    parse: function(arg, context) {
      var options = {
        filetype: this.filetype,
        existing: this.existing,
        matches: this.matches
      };
      var promise = fileparser.parse(context, arg.text, options);

      return promise.then(function(reply) {
        return new Conversion(reply.value, arg, reply.status,
                              reply.message, reply.predictor);
      });
    }
  }
];
