


"use strict";

let { Ci } = require("chrome");
let WebConsoleUtils = require("devtools/toolkit/webconsole/utils").Utils;





loader.lazyRequireGetter(this, "L10N",
  "devtools/shared/timeline/global", true);
loader.lazyRequireGetter(this, "TIMELINE_BLUEPRINT",
  "devtools/shared/timeline/global", true);
loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");
loader.lazyRequireGetter(this, "MarkerUtils",
  "devtools/shared/timeline/marker-utils");









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

  








  render: function({toolbox: toolbox, marker: marker, frames: frames}) {
    this.empty();

    

    let title = MarkerUtils.DOM.buildTitle(this._document, marker);
    let duration = MarkerUtils.DOM.buildDuration(this._document, marker);
    let fields = MarkerUtils.DOM.buildFields(this._document, marker);

    this._parent.appendChild(title);
    this._parent.appendChild(duration);
    fields.forEach(field => this._parent.appendChild(field));

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
          this.emit("view-source", url, line);
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
};

exports.MarkerDetails = MarkerDetails;
