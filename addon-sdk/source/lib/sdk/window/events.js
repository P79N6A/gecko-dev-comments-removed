


"use strict";

module.metadata = {
  "stability": "unstable"
};

const { Ci } = require("chrome");
const events = require("../system/events");
const { on, off, emit } = require("../event/core");
const { windows } = require("../window/utils");



const channel = {};
exports.events = channel;

const types = {
  domwindowopened: "open",
  domwindowclosed: "close",
}


function nsIDOMWindow($) $.QueryInterface(Ci.nsIDOMWindow);







function onOpen(event) {
  observe(nsIDOMWindow(event.subject));
  dispatch(event);
}



function observe(window) {
  function listener(event) {
    if (event.target === window.document) {
      window.removeEventListener(event.type, listener, true);
      emit(channel, "data", { type: event.type, target: window });
    }
  }

  
  
  
  window.addEventListener("DOMContentLoaded", listener, true);
  window.addEventListener("load", listener, true);
  
  
}



function dispatch({ type: topic, subject }) {
  emit(channel, "data", {
    topic: topic,
    type: types[topic],
    target: nsIDOMWindow(subject)
  });
}



let opened = windows(null, { includePrivate: true });
opened.forEach(observe);







events.on("domwindowopened", onOpen);
events.on("domwindowclosed", dispatch);
