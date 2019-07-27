





"use strict";

const { method } = require("devtools/server/protocol");










exports.actorBridge = function actorBridge (methodName, definition={}) {
  return method(function () {
    return this.bridge[methodName].apply(this.bridge, arguments);
  }, definition);
}
