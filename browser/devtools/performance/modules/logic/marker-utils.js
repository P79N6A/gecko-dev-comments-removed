


"use strict";






const { Cu, Ci } = require("chrome");

loader.lazyRequireGetter(this, "L10N",
  "devtools/performance/global", true);
loader.lazyRequireGetter(this, "PREFS",
  "devtools/performance/global", true);
loader.lazyRequireGetter(this, "TIMELINE_BLUEPRINT",
  "devtools/performance/markers", true);
loader.lazyRequireGetter(this, "WebConsoleUtils",
  "devtools/toolkit/webconsole/utils");


const GECKO_SYMBOL = "(Gecko)";





function isMarkerValid (marker, filter) {
  let isUnknown = !(marker.name in TIMELINE_BLUEPRINT);
  if (isUnknown) {
    return filter.indexOf("UNKNOWN") === -1;
  }
  return filter.indexOf(marker.name) === -1;
}








function getMarkerLabel (marker) {
  let blueprint = getBlueprintFor(marker);
  
  
  return typeof blueprint.label === "function" ? blueprint.label(marker) : blueprint.label;
}








function getMarkerClassName (type) {
  let blueprint = getBlueprintFor({ name: type });
  
  
  let className = typeof blueprint.label === "function" ? blueprint.label() : blueprint.label;

  
  
  if (!className) {
    let message = `Could not find marker class name for "${type}".`;
    if (typeof blueprint.label === "function") {
      message += ` The following function must return a class name string when no marker passed: ${blueprint.label}`;
    } else {
      message += ` ${type}.label must be defined in the marker blueprint.`;
    }
    throw new Error(message);
  }

  return className;
}








function getMarkerFields (marker) {
  let blueprint = getBlueprintFor(marker);

  
  if (typeof blueprint.fields === "function") {
    let fields = blueprint.fields(marker);
    
    
    return Object.keys(fields || []).map(label => {
      
      let normalizedLabel = label.indexOf(":") !== -1 ? label : (label + ":");
      return { label: normalizedLabel, value: fields[label] };
    });
  }

  
  return (blueprint.fields || []).reduce((fields, field) => {
    
    if (field.property in marker) {
      let label = field.label;
      let value = marker[field.property];
      fields.push({ label, value });
    }
    return fields;
  }, []);
}




const DOM = {
  







  buildFields: function (doc, marker) {
    let blueprint = getBlueprintFor(marker);
    let fields = getMarkerFields(marker);

    return fields.map(({ label, value }) => DOM.buildNameValueLabel(doc, label, value));
  },

  






  buildTitle: function (doc, marker) {
    let blueprint = getBlueprintFor(marker);

    let hbox = doc.createElement("hbox");
    hbox.setAttribute("align", "center");

    let bullet = doc.createElement("hbox");
    bullet.className = `marker-details-bullet marker-color-${blueprint.colorName}`;

    let title = getMarkerLabel(marker);
    let label = doc.createElement("label");
    label.className = "marker-details-type";
    label.setAttribute("value", title);

    hbox.appendChild(bullet);
    hbox.appendChild(label);

    return hbox;
  },

  






  buildDuration: function (doc, marker) {
    let label = L10N.getStr("timeline.markerDetail.duration");
    let start = L10N.getFormatStrWithNumbers("timeline.tick", marker.start);
    let end = L10N.getFormatStrWithNumbers("timeline.tick", marker.end);
    let duration = L10N.getFormatStrWithNumbers("timeline.tick", marker.end - marker.start);
    let el = DOM.buildNameValueLabel(doc, label, duration);
    el.classList.add("marker-details-duration");
    el.setAttribute("tooltiptext", `${start} â†’ ${end}`);
    return el;
  },

  










  buildNameValueLabel: function (doc, field, value) {
    let hbox = doc.createElement("hbox");
    hbox.className = "marker-details-labelcontainer";
    let labelName = doc.createElement("label");
    let labelValue = doc.createElement("label");
    labelName.className = "plain marker-details-labelname";
    labelValue.className = "plain marker-details-labelvalue";
    labelName.setAttribute("value", field);
    labelValue.setAttribute("value", value);
    hbox.appendChild(labelName);
    hbox.appendChild(labelValue);
    return hbox;
  },

  









  buildStackTrace: function(doc, { type, frameIndex, frames }) {
    let container = doc.createElement("vbox");
    let labelName = doc.createElement("label");
    labelName.className = "plain marker-details-labelname";
    labelName.setAttribute("value", L10N.getStr(`timeline.markerDetail.${type}`));
    container.setAttribute("type", type);
    container.className = "marker-details-stack";
    container.appendChild(labelName);

    let wasAsyncParent = false;
    while (frameIndex > 0) {
      let frame = frames[frameIndex];
      let url = frame.source;
      let displayName = frame.functionDisplayName;
      let line = frame.line;

      
      
      if (wasAsyncParent) {
        let asyncBox = doc.createElement("hbox");
        let asyncLabel = doc.createElement("label");
        asyncLabel.className = "devtools-monospace";
        asyncLabel.setAttribute("value", L10N.getFormatStr("timeline.markerDetail.asyncStack",
                                                           frame.asyncCause));
        asyncBox.appendChild(asyncLabel);
        container.appendChild(asyncBox);
        wasAsyncParent = false;
      }

      let hbox = doc.createElement("hbox");

      if (displayName) {
        let functionLabel = doc.createElement("label");
        functionLabel.className = "devtools-monospace";
        functionLabel.setAttribute("value", displayName);
        hbox.appendChild(functionLabel);
      }

      if (url) {
        let aNode = doc.createElement("a");
        aNode.className = "waterfall-marker-location devtools-source-link";
        aNode.href = url;
        aNode.draggable = false;
        aNode.setAttribute("title", url);

        let urlNode = doc.createElement("label");
        urlNode.className = "filename";
        urlNode.setAttribute("value", WebConsoleUtils.Utils.abbreviateSourceURL(url));
        let lineNode = doc.createElement("label");
        lineNode.className = "line-number";
        lineNode.setAttribute("value", `:${line}`);

        aNode.appendChild(urlNode);
        aNode.appendChild(lineNode);
        hbox.appendChild(aNode);

        
        
        aNode.setAttribute("data-action", JSON.stringify({
          url, line, action: "view-source"
        }));
      }

      if (!displayName && !url) {
        let label = doc.createElement("label");
        label.setAttribute("value", L10N.getStr("timeline.markerDetail.unknownFrame"));
        hbox.appendChild(label);
      }

      container.appendChild(hbox);

      if (frame.asyncParent) {
        frameIndex = frame.asyncParent;
        wasAsyncParent = true;
      } else {
        frameIndex = frame.parent;
      }
    }

    return container;
  }
};



















const CollapseFunctions = {
  


  identical: function (parent, curr, peek) {
    let next = peek(1);
    
    
    if (parent && parent.name == curr.name) {
      let finalize = next && next.name !== curr.name;
      return { collapse: true, finalize };
    }
    
    
    if (next && curr.name == next.name) {
      return { toParent: { name: curr.name, start: curr.start }, collapse: true };
    }
  },

  


  adjacent: function (parent, curr, peek) {
    let next = peek(1);
    if (next && (next.start < curr.end || next.start - curr.end <= 10 )) {
      return CollapseFunctions.identical(parent, curr, peek);
    }
  },

  



  child: function (parent, curr, peek) {
    let next = peek(1);
    
    if (parent && curr.end <= parent.end) {
      let finalize = next && next.end > parent.end;
      return { collapse: true, finalize };
    }
  },

  



  parent: function (parent, curr, peek) {
    let next = peek(1);
    
    
    if (next && curr.end >= next.end) {
      return { toParent: curr };
    }
  },
};





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




const Formatters = {
  



  UnknownLabel: function (marker={}) {
    return marker.name || L10N.getStr("timeline.label.unknown");
  },

  GCLabel: function (marker={}) {
    let label = L10N.getStr("timeline.label.garbageCollection");
    
    
    if ("nonincrementalReason" in marker) {
      label = `${label} (Non-incremental)`;
    }
    return label;
  },

  JSLabel: function (marker={}) {
    let generic = L10N.getStr("timeline.label.javascript2");
    if ("causeName" in marker) {
      return JS_MARKER_MAP[marker.causeName] || generic;
    }
    return generic;
  },

  DOMJSLabel: function (marker={}) {
    return `Event (${marker.type})`;
  },

  





  JSFields: function (marker) {
    if ("causeName" in marker && !JS_MARKER_MAP[marker.causeName]) {
      return { Reason: PREFS["show-platform-data"] ? marker.causeName : GECKO_SYMBOL };
    }
  },

  DOMEventFields: function (marker) {
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
  },

  StylesFields: function (marker) {
    if ("restyleHint" in marker) {
      return { "Restyle Hint": marker.restyleHint.replace(/eRestyle_/g, "") };
    }
  },

  CycleCollectionFields: function (marker) {
    let Type = PREFS["show-platform-data"]
        ? marker.name
        : marker.name.replace(/nsCycleCollector::/g, "");
    return { Type };
  },
};








function getBlueprintFor (marker) {
  return TIMELINE_BLUEPRINT[marker.name] || TIMELINE_BLUEPRINT.UNKNOWN;
}

exports.isMarkerValid = isMarkerValid;
exports.getMarkerLabel = getMarkerLabel;
exports.getMarkerClassName = getMarkerClassName;
exports.getMarkerFields = getMarkerFields;
exports.DOM = DOM;
exports.CollapseFunctions = CollapseFunctions;
exports.Formatters = Formatters;
exports.getBlueprintFor = getBlueprintFor;
