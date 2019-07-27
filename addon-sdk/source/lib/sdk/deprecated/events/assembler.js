



"use strict";

const { Class } = require("../../core/heritage");
const { removeListener, on } = require("../../dom/events");









exports.DOMEventAssembler = Class({
  






  handleEvent() {
    throw new TypeError("Instance of DOMEventAssembler must implement `handleEvent` method");
  },
  



  get supportedEventsTypes() {
    throw new TypeError("Instance of DOMEventAssembler must implement `handleEvent` field");
  },
  




  observe: function observe(eventTarget) {
    this.supportedEventsTypes.forEach(function(eventType) {
      on(eventTarget, eventType, this);
    }, this);
  },
  




  ignore: function ignore(eventTarget) {
    this.supportedEventsTypes.forEach(function(eventType) {
      removeListener(eventTarget, eventType, this);
    }, this);
  }
});
