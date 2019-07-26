


"use strict";

const { Cu } = require("chrome");




try {
  Cu.import("resource:///modules/CustomizableUI.jsm", {});
}
catch (e) {
  throw Error("Unsupported Application: The module"  + module.id +
              " does not support this application.");
}

const { CustomizableUI } = Cu.import('resource:///modules/CustomizableUI.jsm', {});
const { receive } = require("../event/utils");
const { InputPort } = require("./system");
const { object} = require("../util/sequence");
const { getOuterId } = require("../window/utils");

const Input = function() {};
Input.prototype = Object.create(InputPort.prototype);

Input.prototype.onCustomizeStart = function (window) {
  receive(this, object([getOuterId(window), true]));
}

Input.prototype.onCustomizeEnd = function (window) {
  receive(this, object([getOuterId(window), null]));
}

Input.prototype.addListener = input => CustomizableUI.addListener(input);

Input.prototype.removeListener = input => CustomizableUI.removeListener(input);

exports.CustomizationInput = Input;
