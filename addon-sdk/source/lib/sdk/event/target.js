



'use strict';

module.metadata = {
  "stability": "stable"
};

const { on, once, off, setListeners } = require('./core');
const { method, chain } = require('../lang/functional');
const { Class } = require('../core/heritage');






const EventTarget = Class({
  




  




  initialize: function initialize(options) {
    setListeners(this, options);
  },
  











  on: chain(method(on)),
  







  once: chain(method(once)),
  






  removeListener: function removeListener(type, listener) {
    
    
    
    
    off(this, type, listener);
    return this;
  },
  off: function(type, listener) {
    off(this, type, listener);
    return this;
  }
});
exports.EventTarget = EventTarget;
