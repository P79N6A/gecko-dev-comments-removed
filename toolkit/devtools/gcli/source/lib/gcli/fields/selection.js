















'use strict';

var util = require('../util/util');
var Menu = require('../ui/menu').Menu;

var Argument = require('../types/types').Argument;
var Conversion = require('../types/types').Conversion;
var Field = require('./fields').Field;




function SelectionField(type, options) {
  Field.call(this, type, options);

  this.arg = new Argument();

  this.menu = new Menu({
    document: this.document,
    maxPredictions: Conversion.maxPredictions
  });
  this.element = this.menu.element;

  this.onFieldChange = util.createEvent('SelectionField.onFieldChange');

  
  this.menu.onItemClick.add(this.itemClicked, this);
}

SelectionField.prototype = Object.create(Field.prototype);

SelectionField.claim = function(type, context) {
  if (context == null) {
    return Field.NO_MATCH;
  }
  return type.getType(context).hasPredictions ? Field.DEFAULT : Field.NO_MATCH;
};

SelectionField.prototype.destroy = function() {
  this.menu.onItemClick.remove(this.itemClicked, this);
  this.menu.destroy();
  this.menu = undefined;
  this.element = undefined;
  Field.prototype.destroy.call(this);
};

SelectionField.prototype.setConversion = function(conversion) {
  this.arg = conversion.arg;
  this.setMessage(conversion.message);

  var context = this.requisition.executionContext;
  conversion.getPredictions(context).then(function(predictions) {
    var items = predictions.map(function(prediction) {
      
      
      
      return prediction.value && prediction.value.description ?
          prediction.value :
          prediction;
    }, this);
    if (this.menu != null) {
      this.menu.show(items, conversion.arg.text);
    }
  }.bind(this)).catch(util.errorHandler);
};

SelectionField.prototype.itemClicked = function(ev) {
  var arg = new Argument(ev.name, '', ' ');
  var context = this.requisition.executionContext;

  this.type.parse(arg, context).then(function(conversion) {
    this.onFieldChange({ conversion: conversion });
    this.setMessage(conversion.message);
  }.bind(this)).catch(util.errorHandler);
};

SelectionField.prototype.getConversion = function() {
  
  this.arg = this.arg.beget({ text: this.input.value });
  return this.type.parse(this.arg, this.requisition.executionContext);
};






SelectionField.prototype.selectChoice = function() {
  var selected = this.menu.selected;
  if (selected == null) {
    return false;
  }

  this.itemClicked({ name: selected });
  return true;
};

Object.defineProperty(SelectionField.prototype, 'isImportant', {
  get: function() {
    return this.type.name !== 'command';
  },
  enumerable: true
});




exports.items = [ SelectionField ];
