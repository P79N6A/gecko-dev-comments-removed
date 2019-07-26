















'use strict';

var util = require('../util/util');











function Field(type, options) {
  this.type = type;
  this.document = options.document;
  this.requisition = options.requisition;
}




Field.prototype.item = 'field';






Field.prototype.element = undefined;





Field.prototype.update = function() {
};




Field.prototype.destroy = function() {
  this.messageElement = undefined;
  this.document = undefined;
  this.requisition = undefined;
};








Field.prototype.setConversion = function(conversion) {
  throw new Error('Field should not be used directly');
};





Field.prototype.getConversion = function() {
  throw new Error('Field should not be used directly');
};





Field.prototype.setMessageElement = function(element) {
  this.messageElement = element;
};




Field.prototype.setMessage = function(message) {
  if (this.messageElement) {
    util.setTextContent(this.messageElement, message || '');
  }
};





Field.prototype.isImportant = false;






Field.claim = function(type, context) {
  throw new Error('Field should not be used directly');
};




Field.MATCH = 3;           
Field.DEFAULT = 2;         
Field.BASIC = 1;           
Field.NO_MATCH = 0;        

exports.Field = Field;





var fieldCtors = [];





exports.addField = function(fieldCtor) {
  if (typeof fieldCtor !== 'function') {
    console.error('addField erroring on ', fieldCtor);
    throw new Error('addField requires a Field constructor');
  }
  fieldCtors.push(fieldCtor);
};






exports.removeField = function(field) {
  if (typeof field !== 'string') {
    fieldCtors = fieldCtors.filter(function(test) {
      return test !== field;
    });
  }
  else if (field instanceof Field) {
    exports.removeField(field.name);
  }
  else {
    console.error('removeField erroring on ', field);
    throw new Error('removeField requires an instance of Field');
  }
};











exports.getField = function(type, options) {
  var FieldConstructor;
  var highestClaim = -1;
  fieldCtors.forEach(function(fieldCtor) {
    var context = (options.requisition == null) ?
                  null : options.requisition.executionContext;
    var claim = fieldCtor.claim(type, context);
    if (claim > highestClaim) {
      highestClaim = claim;
      FieldConstructor = fieldCtor;
    }
  });

  if (!FieldConstructor) {
    console.error('Unknown field type ', type, ' in ', fieldCtors);
    throw new Error('Can\'t find field for ' + type);
  }

  if (highestClaim < Field.DEFAULT) {
    return new BlankField(type, options);
  }

  return new FieldConstructor(type, options);
};






function BlankField(type, options) {
  Field.call(this, type, options);

  this.element = util.createElement(this.document, 'div');

  this.onFieldChange = util.createEvent('BlankField.onFieldChange');
}

BlankField.prototype = Object.create(Field.prototype);

BlankField.claim = function(type, context) {
  return type.name === 'blank' ? Field.MATCH : Field.NO_MATCH;
};

BlankField.prototype.destroy = function() {
  this.element = undefined;
  Field.prototype.destroy.call(this);
};

BlankField.prototype.setConversion = function(conversion) {
  this.setMessage(conversion.message);
};

BlankField.prototype.getConversion = function() {
  return this.type.parseString('', this.requisition.executionContext);
};

exports.addField(BlankField);
