


"use strict";






const { Cc, Ci, Cu, Cr } = require("chrome");
const { Heritage } = require("resource:///modules/devtools/ViewHelpers.jsm");
const { AbstractTreeItem } = require("resource:///modules/devtools/AbstractTreeItem.jsm");
const { TIMELINE_BLUEPRINT: ORIGINAL_BP } = require("devtools/performance/markers");

loader.lazyRequireGetter(this, "MarkerUtils",
  "devtools/performance/marker-utils");

const HTML_NS = "http://www.w3.org/1999/xhtml";

const LEVEL_INDENT = 10; 
const ARROW_NODE_OFFSET = -15; 
const WATERFALL_MARKER_SIDEBAR_WIDTH = 175; 
const WATERFALL_MARKER_TIMEBAR_WIDTH_MIN = 5; 















function MarkerView({ owner, marker, level, hidden }) {
  AbstractTreeItem.call(this, {
    parent: owner,
    level: level|0 - (hidden ? 1 : 0)
  });

  this.marker = marker;
  this.hidden = !!hidden;

  this._onItemBlur = this._onItemBlur.bind(this);
  this._onItemFocus = this._onItemFocus.bind(this);
}

MarkerView.prototype = Heritage.extend(AbstractTreeItem.prototype, {
  



  recalculateBounds: function() {
    this.root._waterfallWidth = this.bounds.width - WATERFALL_MARKER_SIDEBAR_WIDTH;
  },

  




  set blueprint(blueprint) {
    this.root._blueprint = blueprint;
  },
  get blueprint() {
    return this.root._blueprint;
  },

  



  set interval(interval) {
    this.root._interval = interval;
  },
  get interval() {
    return this.root._interval;
  },

  



  getWaterfallWidth: function() {
    return this._waterfallWidth;
  },

  



  getDataScale: function() {
    let startTime = this.root._interval.startTime|0;
    let endTime = this.root._interval.endTime|0;
    return this.root._waterfallWidth / (endTime - startTime);
  },

  





  _displaySelf: function(document, arrowNode) {
    let targetNode = document.createElement("hbox");
    targetNode.className = "waterfall-tree-item";

    if (this == this.root) {
      
      
      
      this.root.recalculateBounds();
      
      
      this._addEventListeners();
    } else {
      
      this._buildMarkerCells(document, targetNode, arrowNode);
    }

    if (this.hidden) {
      targetNode.style.display = "none";
    }

    return targetNode;
  },

  



  _populateSelf: function(children) {
    let submarkers = this.marker.submarkers;
    if (!submarkers || !submarkers.length) {
      return;
    }
    let blueprint = this.root._blueprint;
    let startTime = this.root._interval.startTime;
    let endTime = this.root._interval.endTime;
    let newLevel = this.level + 1;

    for (let i = 0, len = submarkers.length; i < len; i++) {
      let marker = submarkers[i];

      
      
      if (!(marker.name in blueprint)) {
        if (!(marker.name in ORIGINAL_BP)) {
          console.warn(`Marker not found in timeline blueprint: ${marker.name}.`);
        }
        continue;
      }
      if (!isMarkerInRange(marker, startTime|0, endTime|0)) {
        continue;
      }
      children.push(new MarkerView({
        owner: this,
        marker: marker,
        level: newLevel,
        inverted: this.inverted
      }));
    }
  },

  





  _buildMarkerCells: function(doc, targetNode, arrowNode) {
    let marker = this.marker;
    let style = this.root._blueprint[marker.name];
    let startTime = this.root._interval.startTime;
    let endTime = this.root._interval.endTime;

    let sidebarCell = this._buildMarkerSidebar(
      doc, style, marker);

    let timebarCell = this._buildMarkerTimebar(
      doc, style, marker, startTime, endTime, arrowNode);

    targetNode.appendChild(sidebarCell);
    targetNode.appendChild(timebarCell);

    
    let submarkers = this.marker.submarkers;
    let hasDescendants = submarkers && submarkers.length > 0;
    if (hasDescendants) {
      targetNode.setAttribute("expandable", "");
    } else {
      arrowNode.setAttribute("invisible", "");
    }

    targetNode.setAttribute("level", this.level);
  },

  



  _buildMarkerSidebar: function(doc, style, marker) {
    let cell = doc.createElement("hbox");
    cell.className = "waterfall-sidebar theme-sidebar";
    cell.setAttribute("width", WATERFALL_MARKER_SIDEBAR_WIDTH);
    cell.setAttribute("align", "center");

    let bullet = doc.createElement("hbox");
    bullet.className = `waterfall-marker-bullet marker-color-${style.colorName}`;
    bullet.style.transform = `translateX(${this.level * LEVEL_INDENT}px)`;
    bullet.setAttribute("type", marker.name);
    cell.appendChild(bullet);

    let name = doc.createElement("description");
    let label = MarkerUtils.getMarkerLabel(marker);
    name.className = "plain waterfall-marker-name";
    name.style.transform = `translateX(${this.level * LEVEL_INDENT}px)`;
    name.setAttribute("crop", "end");
    name.setAttribute("flex", "1");
    name.setAttribute("value", label);
    name.setAttribute("tooltiptext", label);
    cell.appendChild(name);

    return cell;
  },
  _buildMarkerTimebar: function(doc, style, marker, startTime, endTime, arrowNode) {
    let cell = doc.createElement("hbox");
    cell.className = "waterfall-marker waterfall-background-ticks";
    cell.setAttribute("align", "center");
    cell.setAttribute("flex", "1");

    let dataScale = this.getDataScale();
    let offset = (marker.start - startTime) * dataScale;
    let width = (marker.end - marker.start) * dataScale;

    arrowNode.style.transform =`translateX(${offset + ARROW_NODE_OFFSET}px)`;
    cell.appendChild(arrowNode);

    let bar = doc.createElement("hbox");
    bar.className = `waterfall-marker-bar marker-color-${style.colorName}`;
    bar.style.transform = `translateX(${offset}px)`;
    bar.setAttribute("type", marker.name);
    bar.setAttribute("width", Math.max(width, WATERFALL_MARKER_TIMEBAR_WIDTH_MIN));
    cell.appendChild(bar);

    return cell;
  },

  


  _addEventListeners: function() {
    this.on("focus", this._onItemFocus);
    this.on("blur", this._onItemBlur);
  },

  


  _onItemBlur: function() {
    this.root.emit("unselected");
  },

  


  _onItemFocus: function(e, item) {
    this.root.emit("selected", item.marker);
  }
});













function isMarkerInRange(e, start, end) {
  let m_start = e.start|0;
  let m_end = e.end|0;

  return (m_start >= start && m_end <= end) || 
         (m_start < start && m_end > end) || 
         (m_start < start && m_end >= start && m_end <= end) || 
         (m_end > end && m_start >= start && m_start <= end); 
}

exports.MarkerView = MarkerView;
exports.WATERFALL_MARKER_SIDEBAR_WIDTH = WATERFALL_MARKER_SIDEBAR_WIDTH;
