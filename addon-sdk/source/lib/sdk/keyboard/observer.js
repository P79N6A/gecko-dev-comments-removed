



"use strict";

module.metadata = {
  "stability": "unstable"
};

const { Class } = require("../core/heritage");
const { EventTarget } = require("../event/target");
const { emit } = require("../event/core");
const { DOMEventAssembler } = require("../deprecated/events/assembler");
const { browserWindowIterator } = require('../deprecated/window-utils');
const { isBrowser } = require('../window/utils');
const { observer: windowObserver } = require("../windows/observer");



const Observer = Class({
  implements: [DOMEventAssembler, EventTarget],
  initialize() {
    
    windowObserver.on("open", window => {
      if (isBrowser(window))
        this.observe(window);
    });

    
    windowObserver.on("close", window => {
      if (isBrowser(window))
        this.ignore(window);
    });

    
    for (let window of browserWindowIterator()) {
      this.observe(window);
    }
  },
  


  supportedEventsTypes: [ "keydown", "keyup", "keypress" ],
  






  handleEvent(event) {
    emit(this, event.type, event, event.target.ownerDocument.defaultView);
  }
});

exports.observer = new Observer();
