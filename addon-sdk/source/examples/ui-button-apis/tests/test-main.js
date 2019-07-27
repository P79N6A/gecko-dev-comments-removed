


"use strict";

try {
  
  var { actionButton, toggleButton, icon } = require("main");
}
catch (e) {
  
  let mainURI = "../lib/main";
  var { actionButton, toggleButton, icon } = require(mainURI);
}
var self = require("sdk/self");

exports.testActionButton = function(assert) {
  assert.equal(actionButton.id, "test-action-button", "action button id is correct");
  assert.equal(actionButton.label, "Action Button", "action button label is correct");
  assert.equal(actionButton.icon, icon, "action button icon is correct");
}

exports.testToggleButton = function(assert) {
  assert.equal(toggleButton.id, "test-toggle-button", "toggle button id is correct");
  assert.equal(toggleButton.label, "Toggle Button", "toggle button label is correct");
  assert.equal(toggleButton.icon, icon, "toggle button icon is correct");
}

require("sdk/test").run(exports);
