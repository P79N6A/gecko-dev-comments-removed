


"use strict";

module.metadata = {
  "stability": "unstable"
};

const { Ci } = require("chrome");
const { observe } = require("../event/chrome");
const { open } = require("../event/dom");
const { windows } = require("../window/utils");
const { filter, merge, map, expand } = require("../event/utils");



function eventsFor(window) {
  let interactive = open(window, "DOMContentLoaded", { capture: true });
  let complete = open(window, "load", { capture: true });
  let states = merge([interactive, complete]);
  let changes = filter(states, function({target}) target === window.document);
  return map(changes, function({type, target}) {
    return { type: type, target: target.defaultView }
  });
}



let opened = windows(null, { includePrivate: true });
let currentEvents = merge(opened.map(eventsFor));


function rename({type, target, data}) {
  return { type: rename[type], target: target, data: data }
}
rename.domwindowopened = "open";
rename.domwindowclosed = "close";

let openEvents = map(observe("domwindowopened"), rename);
let closeEvents = map(observe("domwindowclosed"), rename);
let futureEvents = expand(openEvents, function({target}) eventsFor(target));

let channel = merge([currentEvents, futureEvents,
                     openEvents, closeEvents]);
exports.events = channel;
