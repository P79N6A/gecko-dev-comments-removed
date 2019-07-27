


"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");
const { ViewHelpers } = require("resource:///modules/devtools/ViewHelpers.jsm");


const GECKO_SYMBOL = "(Gecko)";





const L10N = new ViewHelpers.MultiL10N([
  "chrome://browser/locale/devtools/timeline.properties",
  "chrome://browser/locale/devtools/profiler.properties"
]);





const PREFS = new ViewHelpers.Prefs("devtools.performance", {
  "show-platform-data": ["Bool", "ui.show-platform-data"],
  "hidden-markers": ["Json", "timeline.hidden-markers"],
  "memory-sample-probability": ["Float", "memory.sample-probability"],
  "memory-max-log-length": ["Int", "memory.max-log-length"],
  "profiler-buffer-size": ["Int", "profiler.buffer-size"],
  "profiler-sample-frequency": ["Int", "profiler.sample-frequency-khz"],
}, {
  monitorChanges: true
});





const CATEGORIES = [{
  color: "#5e88b0",
  abbrev: "other",
  label: L10N.getStr("category.other")
}, {
  color: "#46afe3",
  abbrev: "css",
  label: L10N.getStr("category.css")
}, {
  color: "#d96629",
  abbrev: "js",
  label: L10N.getStr("category.js")
}, {
  color: "#eb5368",
  abbrev: "gc",
  label: L10N.getStr("category.gc")
}, {
  color: "#df80ff",
  abbrev: "network",
  label: L10N.getStr("category.network")
}, {
  color: "#70bf53",
  abbrev: "graphics",
  label: L10N.getStr("category.graphics")
}, {
  color: "#8fa1b2",
  abbrev: "storage",
  label: L10N.getStr("category.storage")
}, {
  color: "#d99b28",
  abbrev: "events",
  label: L10N.getStr("category.events")
}];





const CATEGORY_MAPPINGS = {
  "16": CATEGORIES[0],    
  "32": CATEGORIES[1],    
  "64": CATEGORIES[2],    
  "128": CATEGORIES[3],   
  "256": CATEGORIES[3],   
  "512": CATEGORIES[4],   
  "1024": CATEGORIES[5],  
  "2048": CATEGORIES[6],  
  "4096": CATEGORIES[7],  
};




































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
    return { Reason: PREFS["show-platform-data"] ? marker.causeName : GECKO_SYMBOL };
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










const [CATEGORY_MASK, CATEGORY_MASK_LIST] = (function () {
  let bitmasksForCategory = {};
  let all = Object.keys(CATEGORY_MAPPINGS);

  for (let category of CATEGORIES) {
    bitmasksForCategory[category.abbrev] = all
      .filter(mask => CATEGORY_MAPPINGS[mask] == category)
      .map(mask => +mask)
      .sort();
  }

  return [
    function (name, index) {
      if (!(name in bitmasksForCategory)) {
        throw new Error(`Category abbreviation '${name}' does not exist.`);
      }
      if (arguments.length == 1) {
        if (bitmasksForCategory[name].length != 1) {
          throw new Error(`Expected exactly one category number for '${name}'.`);
        } else {
          return bitmasksForCategory[name][0];
        }
      } else {
        if (index > bitmasksForCategory[name].length) {
          throw new Error(`Index '${index}' too high for category '${name}'.`);
        } else {
          return bitmasksForCategory[name][index - 1];
        }
      }
    },

    function (name) {
      if (!(name in bitmasksForCategory)) {
        throw new Error(`Category abbreviation '${name}' does not exist.`);
      }
      return bitmasksForCategory[name];
    }
  ];
})();




const CATEGORY_OTHER = CATEGORY_MASK('other');



const CATEGORY_JIT = CATEGORY_MASK('js');


exports.L10N = L10N;
exports.PREFS = PREFS;
exports.TIMELINE_BLUEPRINT = TIMELINE_BLUEPRINT;
exports.CATEGORIES = CATEGORIES;
exports.CATEGORY_MAPPINGS = CATEGORY_MAPPINGS;
exports.CATEGORY_MASK = CATEGORY_MASK;
exports.CATEGORY_MASK_LIST = CATEGORY_MASK_LIST;
exports.CATEGORY_OTHER = CATEGORY_OTHER;
exports.CATEGORY_JIT = CATEGORY_JIT;
