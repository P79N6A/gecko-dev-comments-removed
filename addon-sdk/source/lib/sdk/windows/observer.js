



"use strict";

module.metadata = {
  "stability": "unstable"
};

const { EventEmitterTrait: EventEmitter } = require("../deprecated/events");
const { WindowTracker, windowIterator } = require("../deprecated/window-utils");
const { DOMEventAssembler } = require("../deprecated/events/assembler");
const { Trait } = require("../deprecated/light-traits");



const observer = Trait.compose(DOMEventAssembler, EventEmitter).create({
  



  _emit: Trait.required,
  


  supportedEventsTypes: [ "activate", "deactivate" ],
  






  handleEvent: function handleEvent(event) {
    this._emit(event.type, event.target, event);
  }
});


WindowTracker({
  onTrack: function onTrack(chromeWindow) {
    observer._emit("open", chromeWindow);
    observer.observe(chromeWindow);
  },
  onUntrack: function onUntrack(chromeWindow) {
    observer._emit("close", chromeWindow);
    observer.ignore(chromeWindow);
  }
});

exports.observer = observer;
