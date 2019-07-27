


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");

Cu.import("resource:///modules/devtools/ViewHelpers.jsm");




const STRINGS_URI = "chrome://browser/locale/devtools/timeline.properties";
const L10N = new ViewHelpers.L10N(STRINGS_URI);

































const TIMELINE_BLUEPRINT = {
  
  "Styles": {
    group: 0,
    colorName: "graphs-purple",
    label: L10N.getStr("timeline.label.styles2")
  },
  "Reflow": {
    group: 0,
    colorName: "graphs-purple",
    label: L10N.getStr("timeline.label.reflow2")
  },
  "Paint": {
    group: 0,
    colorName: "graphs-green",
    label: L10N.getStr("timeline.label.paint")
  },

  
  "DOMEvent": {
    group: 1,
    colorName: "graphs-yellow",
    label: L10N.getStr("timeline.label.domevent"),
    fields: [{
      property: "type",
      label: L10N.getStr("timeline.markerDetail.DOMEventType")
    }, {
      property: "eventPhase",
      label: L10N.getStr("timeline.markerDetail.DOMEventPhase"),
      formatter: getEventPhaseName
    }]
  },
  "Javascript": {
    group: 1,
    colorName: "graphs-yellow",
    label: getJSLabel,
  },
  "Parse HTML": {
    group: 1,
    colorName: "graphs-yellow",
    label: L10N.getStr("timeline.label.parseHTML")
  },
  "Parse XML": {
    group: 1,
    colorName: "graphs-yellow",
    label: L10N.getStr("timeline.label.parseXML")
  },
  "GarbageCollection": {
    group: 1,
    colorName: "graphs-red",
    label: getGCLabel,
    fields: [
      { property: "causeName", label: "Reason:" },
      { property: "nonincrementalReason", label: "Non-incremental Reason:" }
    ]
  },

  
  "ConsoleTime": {
    group: 2,
    colorName: "graphs-grey",
    label: L10N.getStr("timeline.label.consoleTime"),
    fields: [{
      property: "causeName",
      label: L10N.getStr("timeline.markerDetail.consoleTimerName")
    }]
  },
  "TimeStamp": {
    group: 2,
    colorName: "graphs-blue",
    label: L10N.getStr("timeline.label.timestamp")
  },
};





function getEventPhaseName (marker) {
  if (marker.eventPhase === Ci.nsIDOMEvent.AT_TARGET) {
    return L10N.getStr("timeline.markerDetail.DOMEventTargetPhase");
  } else if (marker.eventPhase === Ci.nsIDOMEvent.CAPTURING_PHASE) {
    return L10N.getStr("timeline.markerDetail.DOMEventCapturingPhase");
  } else if (marker.eventPhase === Ci.nsIDOMEvent.BUBBLING_PHASE) {
    return L10N.getStr("timeline.markerDetail.DOMEventBubblingPhase");
  }
}

function getGCLabel (marker={}) {
  let label = L10N.getStr("timeline.label.garbageCollection");
  
  
  if ("nonincrementalReason" in marker) {
    label = `${label} (Non-incremental)`;
  }
  return label;
}

function getJSLabel (marker={}) {
  if ("causeName" in marker) {
    return marker.causeName;
  }
  return L10N.getStr("timeline.label.javascript2");
}

function getStylesFields (marker) {
  if ("restyleHint" in marker) {
    return { "Restyle Hint": marker.restyleHint.replace(/eRestyle_/g, "") };
  }
}







function sublabelForProperty (mainLabel, prop) {
  return (marker={}) => marker[prop] ? `${mainLabel} (${marker[prop]})` : mainLabel;
}


exports.L10N = L10N;
exports.TIMELINE_BLUEPRINT = TIMELINE_BLUEPRINT;
