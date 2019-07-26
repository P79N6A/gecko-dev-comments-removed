





'use strict';

module.metadata = {
  "stability": "stable"
};

const { on, once, off, setListeners } = require('./core');
const { method } = require('../lang/functional');
const { Class } = require('../core/heritage');






const EventTarget = Class({
  




  




  initialize: function initialize(options) {
    setListeners(this, options);
  },
  











  on: method(on),
  







  once: method(once),
  






  removeListener: function removeListener(type, listener) {
    
    
    
    
    off(this, type, listener);
  },
  off: function(type, listener) {
    off(this, type, listener)
  }
});
exports.EventTarget = EventTarget;
