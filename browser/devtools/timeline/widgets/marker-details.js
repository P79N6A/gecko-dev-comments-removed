



"use strict";

let { Ci } = require("chrome");





loader.lazyRequireGetter(this, "L10N",
  "devtools/timeline/global", true);
loader.lazyRequireGetter(this, "TIMELINE_BLUEPRINT",
  "devtools/timeline/global", true);
loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");









function MarkerDetails(parent, splitter) {
  EventEmitter.decorate(this);
  this._document = parent.ownerDocument;
  this._parent = parent;
  this._splitter = splitter;
  this._splitter.addEventListener("mouseup", () => this.emit("resize"));
}

MarkerDetails.prototype = {
  


  destroy: function() {
    this.empty();
    this._parent = null;
    this._splitter = null;
  },

  


  empty: function() {
    this._parent.innerHTML = "";
  },

  






  buildMarkerTypeLabel: function(type) {
    let blueprint = TIMELINE_BLUEPRINT[type];

    let hbox = this._document.createElement("hbox");
    hbox.setAttribute("align", "center");

    let bullet = this._document.createElement("hbox");
    bullet.className = "marker-details-bullet";
    bullet.style.backgroundColor = blueprint.fill;
    bullet.style.borderColor = blueprint.stroke;

    let label = this._document.createElement("label");
    label.className = "marker-details-type";
    label.setAttribute("value", blueprint.label);

    hbox.appendChild(bullet);
    hbox.appendChild(label);

    return hbox;
  },

  








  buildNameValueLabel: function(l10nName, value) {
    let hbox = this._document.createElement("hbox");
    let labelName = this._document.createElement("label");
    let labelValue = this._document.createElement("label");
    labelName.className = "plain marker-details-labelname";
    labelValue.className = "plain marker-details-labelvalue";
    labelName.setAttribute("value", L10N.getStr(l10nName));
    labelValue.setAttribute("value", value);
    hbox.appendChild(labelName);
    hbox.appendChild(labelValue);
    return hbox;
  },

  





  render: function(marker) {
    this.empty();

    

    let title = this.buildMarkerTypeLabel(marker.name);

    let toMs = ms => L10N.getFormatStrWithNumbers("timeline.tick", ms);

    let start = this.buildNameValueLabel("timeline.markerDetail.start", toMs(marker.start));
    let end = this.buildNameValueLabel("timeline.markerDetail.end", toMs(marker.end));
    let duration = this.buildNameValueLabel("timeline.markerDetail.duration", toMs(marker.end - marker.start));

    start.classList.add("marker-details-start");
    end.classList.add("marker-details-end");
    duration.classList.add("marker-details-duration");

    this._parent.appendChild(title);
    this._parent.appendChild(start);
    this._parent.appendChild(end);
    this._parent.appendChild(duration);

    

    switch (marker.name) {
      case "ConsoleTime":
        this.renderConsoleTimeMarker(this._parent, marker);
        break;
      case "DOMEvent":
        this.renderDOMEventMarker(this._parent, marker);
        break;
      default:
    }
  },

  







  renderConsoleTimeMarker: function(parent, marker) {
    if ("causeName" in marker) {
      let timerName = this.buildNameValueLabel("timeline.markerDetail.consoleTimerName", marker.causeName);
      this._parent.appendChild(timerName);
    }
  },

  







  renderDOMEventMarker: function(parent, marker) {
    if ("type" in marker) {
      let type = this.buildNameValueLabel("timeline.markerDetail.DOMEventType", marker.type);
      this._parent.appendChild(type);
    }
    if ("eventPhase" in marker) {
      let phaseL10NProp;
      if (marker.eventPhase == Ci.nsIDOMEvent.AT_TARGET) {
        phaseL10NProp = "timeline.markerDetail.DOMEventTargetPhase";
      }
      if (marker.eventPhase == Ci.nsIDOMEvent.CAPTURING_PHASE) {
        phaseL10NProp = "timeline.markerDetail.DOMEventCapturingPhase";
      }
      if (marker.eventPhase == Ci.nsIDOMEvent.BUBBLING_PHASE) {
        phaseL10NProp = "timeline.markerDetail.DOMEventBubblingPhase";
      }
      let phase = this.buildNameValueLabel("timeline.markerDetail.DOMEventPhase", L10N.getStr(phaseL10NProp));
      this._parent.appendChild(phase);
    }
  },

}

exports.MarkerDetails = MarkerDetails;
