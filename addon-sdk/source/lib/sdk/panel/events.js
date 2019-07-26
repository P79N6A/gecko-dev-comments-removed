



"use strict";




module.metadata = {
  "stability": "experimental"
};

const events = require("../system/events");
const { emit } = require("../event/core");

let channel = {};

function forward({ subject, type, data })
  emit(channel, "data", { target: subject, type: type, data: data });

["sdk-panel-show", "sdk-panel-hide", "sdk-panel-shown",
 "sdk-panel-hidden", "sdk-panel-content-changed", "sdk-panel-content-loaded",
 "sdk-panel-document-loaded"
].forEach(function(type) events.on(type, forward));

exports.events = channel;
