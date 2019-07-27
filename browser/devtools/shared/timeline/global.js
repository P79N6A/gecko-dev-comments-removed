


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");
loader.lazyRequireGetter(this, "ViewHelpers",
  "resource:///modules/devtools/ViewHelpers.jsm", true);
loader.lazyRequireGetter(this, "Services");


const GECKO_SYMBOL = "(Gecko)";




const STRINGS_URI = "chrome://browser/locale/devtools/timeline.properties";
const L10N = new ViewHelpers.L10N(STRINGS_URI);




const prefs = new ViewHelpers.Prefs("devtools.performance.ui", {
  showPlatformData: ["Bool", "show-platform-data"]
});

let SHOW_PLATFORM_DATA = Services.prefs.getBoolPref("devtools.performance.ui.show-platform-data");
prefs.registerObserver();
prefs.on("pref-changed", (_,  prefName, prefValue) => {
  if (prefName === "showPlatformData") {
    SHOW_PLATFORM_DATA = prefValue;
  }
});

































const TIMELINE_BLUEPRINT = {
  
  "Styles": {
    group: 0,
    colorName: "graphs-purple",
    label: L10N.getStr("timeline.label.styles2"),
    fields: getStylesFields,
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
    fields: getDOMEventFields,
  },
  "Javascript": {
    group: 1,
    colorName: "graphs-yellow",
    label: getJSLabel,
    fields: getJSFields,
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
    label: sublabelForProperty(L10N.getStr("timeline.label.consoleTime"), "causeName"),
    fields: [{
      property: "causeName",
      label: L10N.getStr("timeline.markerDetail.consoleTimerName")
    }]
  },
  "TimeStamp": {
    group: 2,
    colorName: "graphs-blue",
    label: sublabelForProperty(L10N.getStr("timeline.label.timestamp"), "causeName"),
    fields: [{
      property: "causeName",
      label: "Label:"
    }]
  },
};





function getGCLabel (marker={}) {
  let label = L10N.getStr("timeline.label.garbageCollection");
  
  
  if ("nonincrementalReason" in marker) {
    label = `${label} (Non-incremental)`;
  }
  return label;
}





const JS_MARKER_MAP = {
  "<script> element":          "Script Tag",
  "setInterval handler":       "setInterval",
  "setTimeout handler":        "setTimeout",
  "FrameRequestCallback":      "requestAnimationFrame",
  "promise callback":          "Promise Callback",
  "promise initializer":       "Promise Init",
  "Worker runnable":           "Worker",
  "javascript: URI":           "JavaScript URI",
  
  
  "EventHandlerNonNull":       "Event Handler",
  "EventListener.handleEvent": "Event Handler",
};

function getJSLabel (marker={}) {
  let generic = L10N.getStr("timeline.label.javascript2");
  if ("causeName" in marker) {
    return JS_MARKER_MAP[marker.causeName] || generic;
  }
  return generic;
}







function getJSFields (marker) {
  if ("causeName" in marker && !JS_MARKER_MAP[marker.causeName]) {
    return { Reason: (SHOW_PLATFORM_DATA ? marker.causeName : GECKO_SYMBOL) };
  }
}

function getDOMEventFields (marker) {
  let fields = Object.create(null);
  if ("type" in marker) {
    fields[L10N.getStr("timeline.markerDetail.DOMEventType")] = marker.type;
  }
  if ("eventPhase" in marker) {
    let phase;
    if (marker.eventPhase === Ci.nsIDOMEvent.AT_TARGET) {
      phase = L10N.getStr("timeline.markerDetail.DOMEventTargetPhase");
    } else if (marker.eventPhase === Ci.nsIDOMEvent.CAPTURING_PHASE) {
      phase = L10N.getStr("timeline.markerDetail.DOMEventCapturingPhase");
    } else if (marker.eventPhase === Ci.nsIDOMEvent.BUBBLING_PHASE) {
      phase = L10N.getStr("timeline.markerDetail.DOMEventBubblingPhase");
    }
    fields[L10N.getStr("timeline.markerDetail.DOMEventPhase")] = phase;
  }
  return fields;
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
