


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");

Cu.import("resource:///modules/devtools/ViewHelpers.jsm");




const STRINGS_URI = "chrome://browser/locale/devtools/timeline.properties";
const L10N = new ViewHelpers.L10N(STRINGS_URI);













const TIMELINE_BLUEPRINT = {
  "Styles": {
    group: 0,
    fill: "hsl(285,50%,68%)",
    stroke: "hsl(285,50%,48%)",
    label: L10N.getStr("timeline.label.styles2")
  },
  "Reflow": {
    group: 0,
    fill: "hsl(285,50%,68%)",
    stroke: "hsl(285,50%,48%)",
    label: L10N.getStr("timeline.label.reflow2")
  },
  "Paint": {
    group: 0,
    fill: "hsl(104,57%,71%)",
    stroke: "hsl(104,57%,51%)",
    label: L10N.getStr("timeline.label.paint")
  },
  "DOMEvent": {
    group: 1,
    fill: "hsl(39,82%,69%)",
    stroke: "hsl(39,82%,49%)",
    label: L10N.getStr("timeline.label.domevent")
  },
  "Javascript": {
    group: 1,
    fill: "hsl(39,82%,69%)",
    stroke: "hsl(39,82%,49%)",
    label: L10N.getStr("timeline.label.javascript2")
  },
  "ConsoleTime": {
    group: 2,
    fill: "hsl(0,0%,80%)",
    stroke: "hsl(0,0%,60%)",
    label: L10N.getStr("timeline.label.consoleTime")
  },
};


exports.L10N = L10N;
exports.TIMELINE_BLUEPRINT = TIMELINE_BLUEPRINT;
