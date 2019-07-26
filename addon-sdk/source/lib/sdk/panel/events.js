



"use strict";




module.metadata = {
  "stability": "experimental"
};

const events = require("../system/events");
const { emit } = require("../event/core");

let channel = {};

function forward({ subject, type, data })
  emit(channel, "data", { target: subject, type: type, data: data });

["popupshowing", "popuphiding", "popupshown", "popuphidden",
"document-element-inserted", "DOMContentLoaded", "load"
].forEach(function(type) events.on(type, forward));

exports.events = channel;
