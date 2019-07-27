


'use strict';

module.metadata = {
  "stability": "unstable"
};

const { EventTarget } = require("../event/target");
const { emit } = require("../event/core");
const { DOMEventAssembler } = require("../deprecated/events/assembler");
const { Class } = require("../core/heritage");
const { getActiveTab, getTabs } = require("./utils");
const { browserWindowIterator } = require("../deprecated/window-utils");
const { isBrowser, windows, getMostRecentBrowserWindow } = require("../window/utils");
const { observer: windowObserver } = require("../windows/observer");

const EVENTS = {
  "TabOpen": "open",
  "TabClose": "close",
  "TabSelect": "select",
  "TabMove": "move",
  "TabPinned": "pinned",
  "TabUnpinned": "unpinned"
};

const selectedTab = Symbol("observer/state/selectedTab");



const Observer = Class({
  implements: [EventTarget, DOMEventAssembler],
  initialize() {
    this[selectedTab] = null;
    
    
    
    
    this.on("select", tab => {
      const selected = this[selectedTab];
      if (selected !== tab) {
        if (selected) {
          emit(this, 'deactivate', selected);
        }

        if (tab) {
          this[selectedTab] = tab;
          emit(this, 'activate', this[selectedTab]);
        }
      }
    });


    
    
    windowObserver.on("open", chromeWindow => {
      if (isBrowser(chromeWindow)) {
        this.observe(chromeWindow);
      }
    });

    windowObserver.on("close", chromeWindow => {
      if (isBrowser(chromeWindow)) {
        
        
        if (getActiveTab(chromeWindow) === this[selectedTab]) {
          emit(this, "deactivate", this[selectedTab]);
          this[selectedTab] = null;
        }
        this.ignore(chromeWindow);
      }
    });


    
    
    
    windowObserver.on("activate", chromeWindow => {
      if (isBrowser(chromeWindow)) {
        emit(this, "select", getActiveTab(chromeWindow));
      }
    });

    
    
    for (let chromeWindow of browserWindowIterator()) {
      this.observe(chromeWindow);
    }
  },
  


  supportedEventsTypes: Object.keys(EVENTS),
  






  handleEvent: function handleEvent(event) {
    emit(this, EVENTS[event.type], event.target, event);
  }
});

exports.observer = new Observer();
