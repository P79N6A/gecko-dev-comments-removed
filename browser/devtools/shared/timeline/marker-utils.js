


"use strict";






loader.lazyRequireGetter(this, "L10N",
  "devtools/shared/timeline/global", true);
loader.lazyRequireGetter(this, "TIMELINE_BLUEPRINT",
  "devtools/shared/timeline/global", true);








function getMarkerLabel (marker) {
  let blueprint = TIMELINE_BLUEPRINT[marker.name];
  
  
  return typeof blueprint.label === "function" ? blueprint.label(marker) : blueprint.label;
}
exports.getMarkerLabel = getMarkerLabel;








function getMarkerFields (marker) {
  let blueprint = TIMELINE_BLUEPRINT[marker.name];
  return (blueprint.fields || []).reduce((fields, field) => {
    
    if (field.property in marker) {
      let label = field.label;
      let value = marker[field.property];
      
      
      if (typeof field.formatter === "function") {
        value = field.formatter(marker);
      }
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
    bullet.className = `marker-details-bullet ${blueprint.colorName}`;

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
    let value = L10N.getFormatStrWithNumbers("timeline.tick", marker.end - marker.start);
    let el = DOM.buildNameValueLabel(doc, label, value);
    el.classList.add("marker-details-duration");
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
