


'use strict';

module.metadata = {
  "stability": "experimental"
};

const { emit, off } = require("./event/core");
const { when: unload } = require("./system/unload");
const { PrefsTarget } = require("./preferences/event-target");
const { id } = require("./self");
const observers = require("./deprecated/observer-service");

const ADDON_BRANCH = "extensions." + id + ".";
const BUTTON_PRESSED = id + "-cmdPressed";

const target = PrefsTarget({ branchName: ADDON_BRANCH });


function buttonClick(subject, data) {
  emit(target, data);
}
observers.add(BUTTON_PRESSED, buttonClick);


unload(function() {
  observers.remove(BUTTON_PRESSED, buttonClick);
});

module.exports = target;
