


"use strict";

module.metadata = {
  "stability": "unstable"
};

const { window } = require("sdk/addon/window");
exports.MessageChannel = window.MessageChannel;
exports.MessagePort = window.MessagePort;
