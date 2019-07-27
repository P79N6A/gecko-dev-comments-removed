



"use strict";

module.metadata = {
  "stability": "unstable"
};

const { Trait } = require("../deprecated/light-traits");
const { EventEmitterTrait: EventEmitter } = require("../deprecated/events");
const { DOMEventAssembler } = require("../deprecated/events/assembler");
const { browserWindowIterator } = require('../deprecated/window-utils');
const { isBrowser } = require('../window/utils');
const { observer: windowObserver } = require("../windows/observer");



const observer = Trait.compose(DOMEventAssembler, EventEmitter).create({
  



  _emit: Trait.required,
  


  supportedEventsTypes: [ "keydown", "keyup", "keypress" ],
  






  handleEvent: function handleEvent(event) {
    this._emit(event.type, event, event.target.ownerDocument.defaultView);
  }
});


windowObserver.on("open", function onOpen(window) {
  if (isBrowser(window))
    observer.observe(window);
});

windowObserver.on("close", function onClose(window) {
  if (isBrowser(window))
    observer.ignore(window);
});


for (let window of browserWindowIterator())
  observer.observe(window);

exports.observer = observer;
