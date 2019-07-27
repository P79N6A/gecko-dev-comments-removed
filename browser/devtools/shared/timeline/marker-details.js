


"use strict";

let { Ci } = require("chrome");
let WebConsoleUtils = require("devtools/toolkit/webconsole/utils").Utils;





loader.lazyRequireGetter(this, "L10N",
  "devtools/shared/timeline/global", true);
loader.lazyRequireGetter(this, "TIMELINE_BLUEPRINT",
  "devtools/shared/timeline/global", true);
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
    bullet.className = `marker-details-bullet ${blueprint.colorName}`;

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

  








  render: function({toolbox: toolbox, marker: marker, frames: frames}) {
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
      case "Javascript":
        this.renderJavascriptMarker(this._parent, marker);
        break;
      default:
    }

    if (marker.stack) {
      let property = "timeline.markerDetail.stack";
      if (marker.endStack) {
        property = "timeline.markerDetail.startStack";
      }
      this.renderStackTrace({toolbox: toolbox, parent: this._parent, property: property,
                             frameIndex: marker.stack, frames: frames});
    }

    if (marker.endStack) {
      this.renderStackTrace({toolbox: toolbox, parent: this._parent, property: "timeline.markerDetail.endStack",
                             frameIndex: marker.endStack, frames: frames});
    }
  },

  










  renderStackTrace: function({toolbox: toolbox, parent: parent,
                              property: property, frameIndex: frameIndex,
                              frames: frames}) {
    let labelName = this._document.createElement("label");
    labelName.className = "plain marker-details-labelname";
    labelName.setAttribute("value", L10N.getStr(property));
    parent.appendChild(labelName);

    let wasAsyncParent = false;
    while (frameIndex > 0) {
      let frame = frames[frameIndex];
      let url = frame.source;
      let displayName = frame.functionDisplayName;
      let line = frame.line;

      
      
      if (wasAsyncParent) {
        let asyncBox = this._document.createElement("hbox");
        let asyncLabel = this._document.createElement("label");
        asyncLabel.className = "devtools-monospace";
        asyncLabel.setAttribute("value", L10N.getFormatStr("timeline.markerDetail.asyncStack",
                                                           frame.asyncCause));
        asyncBox.appendChild(asyncLabel);
        parent.appendChild(asyncBox);
        wasAsyncParent = false;
      }

      let hbox = this._document.createElement("hbox");

      if (displayName) {
        let functionLabel = this._document.createElement("label");
        functionLabel.className = "devtools-monospace";
        functionLabel.setAttribute("value", displayName);
        hbox.appendChild(functionLabel);
      }

      if (url) {
        let aNode = this._document.createElement("a");
        aNode.className = "waterfall-marker-location theme-link devtools-monospace";
        aNode.href = url;
        aNode.draggable = false;
        aNode.setAttribute("title", url);

        let text = WebConsoleUtils.abbreviateSourceURL(url) + ":" + line;
        let label = this._document.createElement("label");
        label.setAttribute("value", text);
        aNode.appendChild(label);
        hbox.appendChild(aNode);

        aNode.addEventListener("click", (event) => {
          event.preventDefault();
          viewSourceInDebugger(toolbox, url, line);
        });
      }

      if (!displayName && !url) {
        let label = this._document.createElement("label");
        label.setAttribute("value", L10N.getStr("timeline.markerDetail.unknownFrame"));
        hbox.appendChild(label);
      }

      parent.appendChild(hbox);

      if (frame.asyncParent) {
        frameIndex = frame.asyncParent;
        wasAsyncParent = true;
      } else {
        frameIndex = frame.parent;
      }
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

  







  renderJavascriptMarker: function(parent, marker) {
    if ("causeName" in marker) {
      let cause = this.buildNameValueLabel("timeline.markerDetail.causeName", marker.causeName);
      this._parent.appendChild(cause);
    }
  },

};









let viewSourceInDebugger = Task.async(function *(toolbox, url, line) {
  
  
  
  let debuggerAlreadyOpen = toolbox.getPanel("jsdebugger");
  let { panelWin: dbg } = yield toolbox.selectTool("jsdebugger");

  if (!debuggerAlreadyOpen) {
    yield dbg.once(dbg.EVENTS.SOURCES_ADDED);
  }

  let { DebuggerView } = dbg;
  let { Sources } = DebuggerView;

  let item = Sources.getItemForAttachment(a => a.source.url === url);
  if (item) {
    return DebuggerView.setEditorLocation(item.attachment.source.actor, line, { noDebug: true });
  }

  return Promise.reject("Couldn't find the specified source in the debugger.");
});

exports.MarkerDetails = MarkerDetails;
