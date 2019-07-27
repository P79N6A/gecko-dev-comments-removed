



var loop = loop || {};
loop.validate = (function() {
  "use strict";

  






  function difference(arr1, arr2) {
    return arr1.filter(function(item) {
      return arr2.indexOf(item) === -1;
    });
  }

  






  function typeName(obj) {
    if (obj === null) {
      return "null";
    }

    if (typeof obj === "function") {
      return obj.name || obj.toString().match(/^function\s?([^\s(]*)/)[1];
    }

    if (typeof obj.constructor === "function") {
      return typeName(obj.constructor);
    }

    return "unknown";
  }

  





  function Validator(schema) {
    this.schema = schema || {};
  }

  Validator.prototype = {
    






    validate: function(values) {
      this._checkRequiredProperties(values);
      this._checkRequiredTypes(values);
      return values;
    },

    






    _checkRequiredTypes: function(values) {
      Object.keys(this.schema).forEach(function(name) {
        var types = this.schema[name];
        types = Array.isArray(types) ? types : [types];
        if (!this._dependencyMatchTypes(values[name], types)) {
          throw new TypeError("invalid dependency: " + name +
                              "; expected " + types.map(typeName).join(", ") +
                              ", got " + typeName(values[name]));
        }
      }, this);
    },

    






    _checkRequiredProperties: function(values) {
      var definedProperties = Object.keys(values).filter(function(name) {
        return typeof values[name] !== "undefined";
      });
      var diff = difference(Object.keys(this.schema), definedProperties);
      if (diff.length > 0) {
        throw new TypeError("missing required " + diff.join(", "));
      }
    },

    







    _dependencyMatchTypes: function(value, types) {
      return types.some(function(Type) {
        try {
          return typeof Type === "undefined"         || 
                 Type === null && value === null     || 
                 value.constructor == Type           || 
                 Type.prototype.isPrototypeOf(value) || 
                 typeName(value) === typeName(Type);    
        } catch (e) {
          return false;
        }
      });
    }
  };

  return {
    Validator: Validator
  };
})();
