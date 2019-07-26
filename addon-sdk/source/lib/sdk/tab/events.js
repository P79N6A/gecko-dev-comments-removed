



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


let readyEvents = filter(events, function(e) e.type === "DOMContentLoaded");

let futureWindows = map(readyEvents, function(e) e.target);



let eventsFromFuture = expand(futureWindows, tabEventsFor);




let interactiveWindows = windows("navigator:browser", { includePrivate: true }).
                         filter(isInteractive);
let eventsFromInteractive = merge(interactiveWindows.map(tabEventsFor));




exports.events = merge([eventsFromInteractive, eventsFromFuture]);
