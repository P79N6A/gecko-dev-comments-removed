



"use strict";

const { Trait } = require("../light-traits");
const { removeListener, on } = require("../../dom/events");









exports.DOMEventAssembler = Trait({
  






  handleEvent: Trait.required,
  



  supportedEventsTypes: Trait.required,
  




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
