


"use strict";





const { Cc, Ci, Cu, Cr } = require("chrome");

loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");

loader.lazyRequireGetter(this, "L10N",
  "devtools/performance/global", true);
loader.lazyRequireGetter(this, "TIMELINE_BLUEPRINT",
  "devtools/performance/global", true);
loader.lazyRequireGetter(this, "MarkerUtils",
  "devtools/performance/marker-utils");









function MarkerDetails(parent, splitter) {
  EventEmitter.decorate(this);
  this._onClick = this._onClick.bind(this);
  this._document = parent.ownerDocument;
  this._parent = parent;
  this._splitter = splitter;
  this._splitter.addEventListener("mouseup", () => this.emit("resize"));
  this._parent.addEventListener("click", this._onClick);
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

  







  render: function({ marker, frames }) {
    this.empty();

    let elements = [];
    elements.push(MarkerUtils.DOM.buildTitle(this._document, marker));
    elements.push(MarkerUtils.DOM.buildDuration(this._document, marker));
    MarkerUtils.DOM.buildFields(this._document, marker).forEach(field => elements.push(field));

    
    
    if (marker.stack) {
      let type = marker.endStack ? "startStack" : "stack";
      elements.push(MarkerUtils.DOM.buildStackTrace(this._document, {
        frameIndex: marker.stack, frames, type
      }));
    }

    if (marker.endStack) {
      elements.push(MarkerUtils.DOM.buildStackTrace(this._document, {
        frameIndex: marker.endStack, frames, type: "endStack"
      }));
    }

    elements.forEach(el => this._parent.appendChild(el));
  },

  




  _onClick: function (e) {
    let data = findActionFromEvent(e.target);
    if (!data) {
      return;
    }

    if (data.action === "view-source") {
      this.emit("view-source", data.url, data.line);
    }
  },
};











function findActionFromEvent (target, container) {
  let el = target;
  let action;
  while (el !== container) {
    if (action = el.getAttribute("data-action")) {
      return JSON.parse(action);
    }
    el = el.parentNode;
  }
  return null;
}

exports.MarkerDetails = MarkerDetails;
