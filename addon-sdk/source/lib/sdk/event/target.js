





'use strict';

module.metadata = {
  "stability": "stable"
};

const { on, once, off } = require('./core');
const { method } = require('../lang/functional');
const { Class } = require('../core/heritage');

const EVENT_TYPE_PATTERN = /^on([A-Z]\w+$)/;






const EventTarget = Class({
  




  initialize: function initialize(options) {
    options = options || {};
    
    
    Object.keys(options).forEach(function onEach(key) {
      let match = EVENT_TYPE_PATTERN.exec(key);
      let type = match && match[1].toLowerCase();
      let listener = options[key];

      if (type && typeof(listener) === 'function')
        this.on(type, listener);
    }, this);
  },
  











  on: method(on),
  







  once: method(once),
  






  removeListener: function removeListener(type, listener) {
    
    
    
    
    off(this, type, listener);
  }
});
exports.EventTarget = EventTarget;
