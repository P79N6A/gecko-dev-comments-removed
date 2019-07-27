


"use strict";

const { L10N } = require("devtools/performance/global");
const { Formatters } = require("devtools/performance/marker-utils");









































const TIMELINE_BLUEPRINT = {
  


  "UNKNOWN": {
    group: 2,
    colorName: "graphs-grey",
    label: Formatters.UnknownLabel,
  },

  
  "Styles": {
    group: 0,
    colorName: "graphs-purple",
    label: L10N.getStr("timeline.label.styles2"),
    fields: Formatters.StylesFields,
  },
  "Reflow": {
    group: 0,
    colorName: "graphs-purple",
    label: L10N.getStr("timeline.label.reflow2"),
  },
  "Paint": {
    group: 0,
    colorName: "graphs-green",
    label: L10N.getStr("timeline.label.paint"),
  },

  
  "DOMEvent": {
    group: 1,
    colorName: "graphs-yellow",
    label: L10N.getStr("timeline.label.domevent"),
    fields: Formatters.DOMEventFields,
  },
  "Javascript": {
    group: 1,
    colorName: "graphs-yellow",
    label: Formatters.JSLabel,
    fields: Formatters.JSFields
  },
  "Parse HTML": {
    group: 1,
    colorName: "graphs-yellow",
    label: L10N.getStr("timeline.label.parseHTML"),
  },
  "Parse XML": {
    group: 1,
    colorName: "graphs-yellow",
    label: L10N.getStr("timeline.label.parseXML"),
  },
  "GarbageCollection": {
    group: 1,
    colorName: "graphs-red",
    label: Formatters.GCLabel,
    fields: [
      { property: "causeName", label: "Reason:" },
      { property: "nonincrementalReason", label: "Non-incremental Reason:" }
    ],
  },
  "nsCycleCollector::Collect": {
    group: 1,
    colorName: "graphs-red",
    label: "Cycle Collection",
    fields: Formatters.CycleCollectionFields,
  },
  "nsCycleCollector::ForgetSkippable": {
    group: 1,
    colorName: "graphs-red",
    label: "Cycle Collection",
    fields: Formatters.CycleCollectionFields,
  },

  
  "ConsoleTime": {
    group: 2,
    colorName: "graphs-blue",
    label: sublabelForProperty(L10N.getStr("timeline.label.consoleTime"), "causeName"),
    fields: [{
      property: "causeName",
      label: L10N.getStr("timeline.markerDetail.consoleTimerName")
    }],
    nestable: false,
    collapsible: false,
  },
  "TimeStamp": {
    group: 2,
    colorName: "graphs-blue",
    label: sublabelForProperty(L10N.getStr("timeline.label.timestamp"), "causeName"),
    fields: [{
      property: "causeName",
      label: "Label:"
    }],
    collapsible: false,
  },
};







function sublabelForProperty (mainLabel, prop) {
  return (marker={}) => marker[prop] ? `${mainLabel} (${marker[prop]})` : mainLabel;
}


exports.TIMELINE_BLUEPRINT = TIMELINE_BLUEPRINT;
