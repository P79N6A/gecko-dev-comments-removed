


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
    label: L10N.getStr("timeline.label.styles")
  },
  "Reflow": {
    group: 2,
    fill: "hsl(104,57%,71%)",
    stroke: "hsl(104,57%,51%)",
    label: L10N.getStr("timeline.label.reflow")
  },
  "Paint": {
    group: 1,
    fill: "hsl(39,82%,69%)",
    stroke: "hsl(39,82%,49%)",
    label: L10N.getStr("timeline.label.paint")
  },
  "DOMEvent": {
    group: 3,
    fill: "hsl(219,82%,69%)",
    stroke: "hsl(219,82%,69%)",
    label: L10N.getStr("timeline.label.domevent")
  },
  "ConsoleTime": {
    group: 4,
    fill: "hsl(0,0%,80%)",
    stroke: "hsl(0,0%,60%)",
    label: L10N.getStr("timeline.label.consoleTime")
  }
};


exports.L10N = L10N;
exports.TIMELINE_BLUEPRINT = TIMELINE_BLUEPRINT;
