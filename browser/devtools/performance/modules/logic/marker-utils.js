


"use strict";






loader.lazyRequireGetter(this, "L10N",
  "devtools/performance/global", true);
loader.lazyRequireGetter(this, "TIMELINE_BLUEPRINT",
  "devtools/performance/global", true);
loader.lazyRequireGetter(this, "WebConsoleUtils",
  "devtools/toolkit/webconsole/utils");








function getMarkerLabel (marker) {
  let blueprint = TIMELINE_BLUEPRINT[marker.name];
  
  
  return typeof blueprint.label === "function" ? blueprint.label(marker) : blueprint.label;
}
exports.getMarkerLabel = getMarkerLabel;








function getMarkerClassName (type) {
  let blueprint = TIMELINE_BLUEPRINT[type];
  
  
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
exports.getMarkerClassName = getMarkerClassName;








function getMarkerFields (marker) {
  let blueprint = TIMELINE_BLUEPRINT[marker.name];

  
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
exports.getMarkerFields = getMarkerFields;




const DOM = exports.DOM = {
  







  buildFields: function (doc, marker) {
    let blueprint = TIMELINE_BLUEPRINT[marker.name];
    let fields = getMarkerFields(marker);

    return fields.map(({ label, value }) => DOM.buildNameValueLabel(doc, label, value));
  },

  






  buildTitle: function (doc, marker) {
    let blueprint = TIMELINE_BLUEPRINT[marker.name];

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
};
