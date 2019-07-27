


"use strict";

const {onPing} = require("./pong");

exports.onPing = onPing;

exports.onInit = (context) => {
  context.sendAsyncMessage("framescript-manager/ready", {state: "ready"});
  context.addMessageListener("framescript-manager/ping", exports.onPing);
};
