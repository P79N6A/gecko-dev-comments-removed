















'use strict';

var Promise = require('../util/promise').Promise;
var l10n = require('../util/l10n');
var centralTypes = require("./types").centralTypes;
var Conversion = require("./types").Conversion;
var Status = require("./types").Status;

exports.items = [
  {
    
    item: "type",
    name: "union",

    constructor: function() {
      
      
      this.types = this.types.map(function(typeData) {
        typeData.type = centralTypes.createType(typeData);
        typeData.lookup = typeData.lookup;
        return typeData;
      });
    },

    stringify: function(value, context) {
      if (value == null) {
        return "";
      }

      var type = this.types.find(function(typeData) {
        return typeData.name == value.type;
      }).type;

      return type.stringify(value[value.type], context);
    },

    parse: function(arg, context) {
      
      
      
      var self = this;

      var onError = function(i) {
        if (i >= self.types.length) {
          return Promise.reject(new Conversion(undefined, arg, Status.ERROR,
            l10n.lookup("commandParseError")));
        } else {
          return tryNext(i + 1);
        }
      };

      var tryNext = function(i) {
        var type = self.types[i].type;

        try {
          return type.parse(arg, context).then(function(conversion) {
            if (conversion.getStatus() === Status.VALID ||
                conversion.getStatus() === Status.INCOMPLETE) {
              
              
              
              if (conversion.value) {
                var oldConversionValue = conversion.value;
                conversion.value = { type: type.name };
                conversion.value[type.name] = oldConversionValue;
              }
              return conversion;
            } else {
              return onError(i);
            }
          });
        } catch(e) {
          return onError(i);
        }
      };

      return Promise.resolve(tryNext(0));
    },
  }
];
