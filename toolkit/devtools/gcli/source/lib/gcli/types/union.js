















'use strict';

var Promise = require('../util/promise').Promise;
var l10n = require('../util/l10n');
var Conversion = require('./types').Conversion;
var Status = require('./types').Status;

exports.items = [
  {
    
    item: 'type',
    name: 'union',
    hasPredictions: true,

    constructor: function() {
      
      
      this.alternatives = this.alternatives.map(function(typeData) {
        return this.types.createType(typeData);
      }.bind(this));
    },

    getSpec: function(command, param) {
      var spec = { name: 'union', alternatives: [] };
      this.alternatives.forEach(function(type) {
        spec.alternatives.push(type.getSpec(command, param));
      }.bind(this));
      return spec;
    },

    stringify: function(value, context) {
      if (value == null) {
        return '';
      }

      var type = this.alternatives.find(function(typeData) {
        return typeData.name === value.type;
      });

      return type.stringify(value[value.type], context);
    },

    parse: function(arg, context) {
      var conversionPromises = this.alternatives.map(function(type) {
        return type.parse(arg, context);
      }.bind(this));

      return Promise.all(conversionPromises).then(function(conversions) {
        
        var predictionPromises = conversions.map(function(conversion) {
          return conversion.getPredictions(context);
        }.bind(this));

        return Promise.all(predictionPromises).then(function(allPredictions) {
          
          
          var maxIndex = allPredictions.reduce(function(prev, prediction) {
            return Math.max(prev, prediction.length);
          }.bind(this), 0);
          var predictions = [];

          indexLoop:
          for (var index = 0; index < maxIndex; index++) {
            for (var p = 0; p <= allPredictions.length; p++) {
              if (predictions.length >= Conversion.maxPredictions) {
                break indexLoop;
              }

              if (allPredictions[p] != null) {
                var prediction = allPredictions[p][index];
                if (prediction != null && predictions.indexOf(prediction) === -1) {
                  predictions.push(prediction);
                }
              }
            }
          }

          var bestStatus = Status.ERROR;
          var value;
          for (var i = 0; i < conversions.length; i++) {
            var conversion = conversions[i];
            var thisStatus = conversion.getStatus(arg);
            if (thisStatus < bestStatus) {
              bestStatus = thisStatus;
            }
            if (bestStatus === Status.VALID) {
              var type = this.alternatives[i].name;
              value = { type: type };
              value[type] = conversion.value;
              break;
            }
          }

          var msg = (bestStatus === Status.VALID) ?
                    '' :
                    l10n.lookupFormat('typesSelectionNomatch', [ arg.text ]);
          return new Conversion(value, arg, bestStatus, msg, predictions);
        }.bind(this));
      }.bind(this));
    },
  }
];
