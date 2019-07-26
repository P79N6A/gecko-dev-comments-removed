


"use strict";

module.metadata = {
  "stability": "unstable"
};

const ON_PREFIX = "on";
const TAB_PREFIX = "Tab";

const EVENTS = {
  ready: "DOMContentLoaded",
  open: "TabOpen",
  close: "TabClose",
  activate: "TabSelect",
  deactivate: null,
  pinned: "TabPinned",
  unpinned: "TabUnpinned"
}
exports.EVENTS = EVENTS;

Object.keys(EVENTS).forEach(function(name) {
  EVENTS[name] = {
    name: name,
    listener: ON_PREFIX + name.charAt(0).toUpperCase() + name.substr(1),
    dom: EVENTS[name]
  }
});
