



"use strict";





module.metadata = {
  "stability": "experimental"
};

const { Ci } = require("chrome");
const { windows, isInteractive } = require("../window/utils");
const { events } = require("../browser/events");
const { open } = require("../event/dom");
const { filter, map, merge, expand } = require("../event/utils");








const TYPES = ["TabOpen","TabClose","TabSelect","TabMove","TabPinned",
               "TabUnpinned"];



function tabEventsFor(window) {
  
  
  
  let channels = TYPES.map(function(type) open(window, type));
  return merge(channels);
}


let readyEvents = filter(function(e) e.type === "DOMContentLoaded", events);

let futureWindows = map(function(e) e.target, readyEvents);



let eventsFromFuture = expand(tabEventsFor, futureWindows);




let interactiveWindows = windows("navigator:browser", { includePrivate: true }).
                         filter(isInteractive);
let eventsFromInteractive = merge(interactiveWindows.map(tabEventsFor));




exports.events = merge([eventsFromInteractive, eventsFromFuture]);
