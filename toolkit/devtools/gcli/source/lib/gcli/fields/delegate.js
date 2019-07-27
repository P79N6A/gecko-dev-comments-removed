















'use strict';

var util = require('../util/util');
var Field = require('./fields').Field;





function DelegateField(type, options) {
  Field.call(this, type, options);
  this.options = options;

  this.element = util.createElement(this.document, 'div');
  this.update();

  this.onFieldChange = util.createEvent('DelegateField.onFieldChange');
}

DelegateField.prototype = Object.create(Field.prototype);

DelegateField.prototype.update = function() {
  var subtype = this.type.getType(this.options.requisition.executionContext);
  if (typeof subtype.parse !== 'function') {
    subtype = this.options.requisition.system.types.createType(subtype);
  }

  
  
  if (subtype === this.subtype) {
    return;
  }

  if (this.field) {
    this.field.destroy();
  }

  this.subtype = subtype;
  var fields = this.options.requisition.system.fields;
  this.field = fields.get(subtype, this.options);

  util.clearElement(this.element);
  this.element.appendChild(this.field.element);
};

DelegateField.claim = function(type, context) {
  return type.isDelegate ? Field.MATCH : Field.NO_MATCH;
};

DelegateField.prototype.destroy = function() {
  this.element = undefined;
  this.options = undefined;
  if (this.field) {
    this.field.destroy();
  }
  this.subtype = undefined;
  Field.prototype.destroy.call(this);
};

DelegateField.prototype.setConversion = function(conversion) {
  this.field.setConversion(conversion);
};

DelegateField.prototype.getConversion = function() {
  return this.field.getConversion();
};

Object.defineProperty(DelegateField.prototype, 'isImportant', {
  get: function() {
    return this.field.isImportant;
  },
  enumerable: true
});




exports.items = [
  DelegateField
];
