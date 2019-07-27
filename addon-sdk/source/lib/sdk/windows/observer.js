


"use strict";

module.metadata = {
  "stability": "unstable"
};

const { EventTarget } = require("../event/target");
const { emit } = require("../event/core");
const { WindowTracker, windowIterator } = require("../deprecated/window-utils");
const { DOMEventAssembler } = require("../deprecated/events/assembler");
const { Class } = require("../core/heritage");



const Observer = Class({
  initialize() {
    
    WindowTracker({
      onTrack: chromeWindow => {
        emit(this, "open", chromeWindow);
        this.observe(chromeWindow);
      },
      onUntrack: chromeWindow => {
        emit(this, "close", chromeWindow);
        this.ignore(chromeWindow);
      }
    });
  },
  implements: [EventTarget, DOMEventAssembler],
  


  supportedEventsTypes: [ "activate", "deactivate" ],
  






  handleEvent(event) {
    emit(this, event.type, event.target, event);
  }
});

exports.observer = new Observer();
