


'use strict';

module.metadata = {
  "stability": "experimental"
};

const { emit, off } = require("./event/core");
const { PrefsTarget } = require("./preferences/event-target");
const { id } = require("./self");
const { on } = require("./system/events");

const ADDON_BRANCH = "extensions." + id + ".";
const BUTTON_PRESSED = id + "-cmdPressed";

const target = PrefsTarget({ branchName: ADDON_BRANCH });


function buttonClick({ data }) {
  emit(target, data);
}
on(BUTTON_PRESSED, buttonClick);

module.exports = target;
