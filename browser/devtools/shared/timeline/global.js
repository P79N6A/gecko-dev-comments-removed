


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");

Cu.import("resource:///modules/devtools/ViewHelpers.jsm");




const STRINGS_URI = "chrome://browser/locale/devtools/timeline.properties";
const L10N = new ViewHelpers.L10N(STRINGS_URI);















const TIMELINE_BLUEPRINT = {
  "Styles": {
    group: 0,
    colorName: "highlight-pink",
    label: L10N.getStr("timeline.label.styles2")
  },
  "Reflow": {
    group: 0,
    colorName: "highlight-pink",
    label: L10N.getStr("timeline.label.reflow2")
  },
  "Paint": {
    group: 0,
    colorName: "highlight-green",
    label: L10N.getStr("timeline.label.paint")
  },
  "DOMEvent": {
    group: 1,
    colorName: "highlight-lightorange",
    label: L10N.getStr("timeline.label.domevent")
  },
  "Javascript": {
    group: 1,
    colorName: "highlight-lightorange",
    label: L10N.getStr("timeline.label.javascript2")
  },
  "ConsoleTime": {
    group: 2,
    colorName: "highlight-bluegrey",
    label: L10N.getStr("timeline.label.consoleTime")
  },
};


exports.L10N = L10N;
exports.TIMELINE_BLUEPRINT = TIMELINE_BLUEPRINT;
