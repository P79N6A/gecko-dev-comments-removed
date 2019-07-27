


"use strict";

const { L10N } = require("devtools/performance/global");
const { Formatters, CollapseFunctions } = require("devtools/performance/marker-utils");



















































const TIMELINE_BLUEPRINT = {
  
  "Styles": {
    group: 0,
    colorName: "graphs-purple",
    collapseFunc: CollapseFunctions.identical,
    label: L10N.getStr("timeline.label.styles2"),
    fields: Formatters.StylesFields,
  },
  "Reflow": {
    group: 0,
    colorName: "graphs-purple",
    collapseFunc: CollapseFunctions.identical,
    label: L10N.getStr("timeline.label.reflow2"),
  },
  "Paint": {
    group: 0,
    colorName: "graphs-green",
    collapseFunc: CollapseFunctions.identical,
    label: L10N.getStr("timeline.label.paint"),
  },

  
  "DOMEvent": {
    group: 1,
    colorName: "graphs-yellow",
    collapseFunc: CollapseFunctions.DOMtoDOMJS,
    label: L10N.getStr("timeline.label.domevent"),
    fields: Formatters.DOMEventFields,
  },
  "Javascript": {
    group: 1,
    colorName: "graphs-yellow",
    collapseFunc: either(CollapseFunctions.JStoDOMJS, CollapseFunctions.identical),
    label: Formatters.JSLabel,
    fields: Formatters.JSFields
  },
  "meta::DOMEvent+JS": {
    colorName: "graphs-yellow",
    label: Formatters.DOMJSLabel,
    fields: Formatters.DOMJSFields,
  },
  "Parse HTML": {
    group: 1,
    colorName: "graphs-yellow",
    collapseFunc: CollapseFunctions.identical,
    label: L10N.getStr("timeline.label.parseHTML"),
  },
  "Parse XML": {
    group: 1,
    colorName: "graphs-yellow",
    collapseFunc: CollapseFunctions.identical,
    label: L10N.getStr("timeline.label.parseXML"),
  },
  "GarbageCollection": {
    group: 1,
    colorName: "graphs-red",
    collapseFunc: CollapseFunctions.adjacent,
    label: Formatters.GCLabel,
    fields: [
      { property: "causeName", label: "Reason:" },
      { property: "nonincrementalReason", label: "Non-incremental Reason:" }
    ],
  },

  
  "ConsoleTime": {
    group: 2,
    colorName: "graphs-grey",
    label: sublabelForProperty(L10N.getStr("timeline.label.consoleTime"), "causeName"),
    fields: [{
      property: "causeName",
      label: L10N.getStr("timeline.markerDetail.consoleTimerName")
    }],
  },
  "TimeStamp": {
    group: 2,
    colorName: "graphs-blue",
    label: sublabelForProperty(L10N.getStr("timeline.label.timestamp"), "causeName"),
    fields: [{
      property: "causeName",
      label: "Label:"
    }],
  },
};







function either(...fun) {
  return function() {
    for (let f of fun) {
      let result = f.apply(null, arguments);
      if (result !== undefined) return result;
    }
  }
}







function sublabelForProperty (mainLabel, prop) {
  return (marker={}) => marker[prop] ? `${mainLabel} (${marker[prop]})` : mainLabel;
}


exports.TIMELINE_BLUEPRINT = TIMELINE_BLUEPRINT;
