


"use strict";






const { Cc, Ci, Cu, Cr } = require("chrome");
const { L10N } = require("devtools/performance/global");
const { Heritage } = require("resource:///modules/devtools/ViewHelpers.jsm");
const { AbstractTreeItem } = require("resource:///modules/devtools/AbstractTreeItem.jsm");

const MILLISECOND_UNITS = L10N.getStr("table.ms");
const PERCENTAGE_UNITS = L10N.getStr("table.percentage");
const URL_LABEL_TOOLTIP = L10N.getStr("table.url.tooltiptext");
const CALL_TREE_INDENTATION = 16; 

const DEFAULT_SORTING_PREDICATE = (frameA, frameB) => {
  let dataA = frameA.getDisplayedData();
  let dataB = frameB.getDisplayedData();
  if (this.inverted) {
    
    if (dataA.selfPercentage === dataB.selfPercentage) {
      return dataA.totalPercentage < dataB.totalPercentage ? 1 : -1;
    }
    return dataA.selfPercentage < dataB.selfPercentage ? 1 : - 1;
  }
  return dataA.totalPercentage < dataB.totalPercentage ? 1 : -1;
};

const DEFAULT_AUTO_EXPAND_DEPTH = 3; 
const DEFAULT_VISIBLE_CELLS = {
  duration: true,
  percentage: true,
  allocations: false,
  selfDuration: true,
  selfPercentage: true,
  selfAllocations: false,
  samples: true,
  function: true
};

const clamp = (val, min, max) => Math.max(min, Math.min(max, val));
const sum = vals => vals.reduce((a, b) => a + b, 0);









































function CallView({
  caller, frame, level, hidden, inverted,
  sortingPredicate, autoExpandDepth, visibleCells
}) {
  AbstractTreeItem.call(this, {
    parent: caller,
    level: level|0 - (hidden ? 1 : 0)
  });

  this.sortingPredicate = sortingPredicate != null
    ? sortingPredicate
    : caller ? caller.sortingPredicate
             : DEFAULT_SORTING_PREDICATE

  this.autoExpandDepth = autoExpandDepth != null
    ? autoExpandDepth
    : caller ? caller.autoExpandDepth
             : DEFAULT_AUTO_EXPAND_DEPTH;

  this.visibleCells = visibleCells != null
    ? visibleCells
    : caller ? caller.visibleCells
             : Object.create(DEFAULT_VISIBLE_CELLS);

  this.caller = caller;
  this.frame = frame;
  this.hidden = hidden;
  this.inverted = inverted;

  this._onUrlClick = this._onUrlClick.bind(this);
};

CallView.prototype = Heritage.extend(AbstractTreeItem.prototype, {
  





  _displaySelf: function(document, arrowNode) {
    let displayedData = this.getDisplayedData();
    let frameInfo = this.frame.getInfo();

    if (this.visibleCells.duration) {
      var durationCell = this._createTimeCell(document, displayedData.totalDuration);
    }
    if (this.visibleCells.selfDuration) {
      var selfDurationCell = this._createTimeCell(document, displayedData.selfDuration, true);
    }
    if (this.visibleCells.percentage) {
      var percentageCell = this._createExecutionCell(document, displayedData.totalPercentage);
    }
    if (this.visibleCells.selfPercentage) {
      var selfPercentageCell = this._createExecutionCell(document, displayedData.selfPercentage, true);
    }
    if (this.visibleCells.allocations) {
      var allocationsCell = this._createAllocationsCell(document, displayedData.totalAllocations);
    }
    if (this.visibleCells.selfAllocations) {
      var selfAllocationsCell = this._createAllocationsCell(document, displayedData.selfAllocations, true);
    }
    if (this.visibleCells.samples) {
      var samplesCell = this._createSamplesCell(document, displayedData.samples);
    }
    if (this.visibleCells.function) {
      var functionCell = this._createFunctionCell(document, arrowNode, displayedData.name, frameInfo, this.level);
    }

    let targetNode = document.createElement("hbox");
    targetNode.className = "call-tree-item";
    targetNode.setAttribute("origin", frameInfo.isContent ? "content" : "chrome");
    targetNode.setAttribute("category", frameInfo.categoryData.abbrev || "");
    targetNode.setAttribute("tooltiptext", displayedData.tooltiptext);

    if (this.hidden) {
      targetNode.style.display = "none";
    }
    if (this.visibleCells.duration) {
      targetNode.appendChild(durationCell);
    }
    if (this.visibleCells.percentage) {
      targetNode.appendChild(percentageCell);
    }
    if (this.visibleCells.allocations) {
      targetNode.appendChild(allocationsCell);
    }
    if (this.visibleCells.selfDuration) {
      targetNode.appendChild(selfDurationCell);
    }
    if (this.visibleCells.selfPercentage) {
      targetNode.appendChild(selfPercentageCell);
    }
    if (this.visibleCells.selfAllocations) {
      targetNode.appendChild(selfAllocationsCell);
    }
    if (this.visibleCells.samples) {
      targetNode.appendChild(samplesCell);
    }
    if (this.visibleCells.function) {
      targetNode.appendChild(functionCell);
    }

    return targetNode;
  },

  




  _populateSelf: function(children) {
    let newLevel = this.level + 1;

    for (let newFrame of this.frame.calls) {
      children.push(new CallView({
        caller: this,
        frame: newFrame,
        level: newLevel,
        inverted: this.inverted
      }));
    }

    
    
    children.sort(this.sortingPredicate.bind(this));
  },

  



  _createTimeCell: function(doc, duration, isSelf = false) {
    let cell = doc.createElement("description");
    cell.className = "plain call-tree-cell";
    cell.setAttribute("type", isSelf ? "self-duration" : "duration");
    cell.setAttribute("crop", "end");
    cell.setAttribute("value", L10N.numberWithDecimals(duration, 2) + " " + MILLISECOND_UNITS);
    return cell;
  },
  _createExecutionCell: function(doc, percentage, isSelf = false) {
    let cell = doc.createElement("description");
    cell.className = "plain call-tree-cell";
    cell.setAttribute("type", isSelf ? "self-percentage" : "percentage");
    cell.setAttribute("crop", "end");
    cell.setAttribute("value", L10N.numberWithDecimals(percentage, 2) + PERCENTAGE_UNITS);
    return cell;
  },
  _createAllocationsCell: function(doc, count, isSelf = false) {
    let cell = doc.createElement("description");
    cell.className = "plain call-tree-cell";
    cell.setAttribute("type", isSelf ? "self-allocations" : "allocations");
    cell.setAttribute("crop", "end");
    cell.setAttribute("value", count || 0);
    return cell;
  },
  _createSamplesCell: function(doc, count) {
    let cell = doc.createElement("description");
    cell.className = "plain call-tree-cell";
    cell.setAttribute("type", "samples");
    cell.setAttribute("crop", "end");
    cell.setAttribute("value", count || 0);
    return cell;
  },
  _createFunctionCell: function(doc, arrowNode, frameName, frameInfo, frameLevel) {
    let cell = doc.createElement("hbox");
    cell.className = "call-tree-cell";
    cell.style.MozMarginStart = (frameLevel * CALL_TREE_INDENTATION) + "px";
    cell.setAttribute("type", "function");
    cell.appendChild(arrowNode);

    
    
    if (frameName) {
      let nameNode = doc.createElement("description");
      nameNode.className = "plain call-tree-name";
      nameNode.setAttribute("flex", "1");
      nameNode.setAttribute("crop", "end");
      nameNode.setAttribute("value", frameName);
      cell.appendChild(nameNode);
    }

    
    if (!frameInfo.isMetaCategory) {
      this._appendFunctionDetailsCells(doc, cell, frameInfo);
    }

    
    let hasDescendants = Object.keys(this.frame.calls).length > 0;
    if (!hasDescendants) {
      arrowNode.setAttribute("invisible", "");
    }

    return cell;
  },
  _appendFunctionDetailsCells: function(doc, cell, frameInfo) {
    if (frameInfo.fileName) {
      let urlNode = doc.createElement("description");
      urlNode.className = "plain call-tree-url";
      urlNode.setAttribute("flex", "1");
      urlNode.setAttribute("crop", "end");
      urlNode.setAttribute("value", frameInfo.fileName);
      urlNode.setAttribute("tooltiptext", URL_LABEL_TOOLTIP + " â†’ " + frameInfo.url);
      urlNode.addEventListener("mousedown", this._onUrlClick);
      cell.appendChild(urlNode);
    }

    if (frameInfo.line) {
      let lineNode = doc.createElement("description");
      lineNode.className = "plain call-tree-line";
      lineNode.setAttribute("value", ":" + frameInfo.line);
      cell.appendChild(lineNode);
    }

    if (frameInfo.column) {
      let columnNode = doc.createElement("description");
      columnNode.className = "plain call-tree-column";
      columnNode.setAttribute("value", ":" + frameInfo.column);
      cell.appendChild(columnNode);
    }

    if (frameInfo.host) {
      let hostNode = doc.createElement("description");
      hostNode.className = "plain call-tree-host";
      hostNode.setAttribute("value", frameInfo.host);
      cell.appendChild(hostNode);
    }

    let spacerNode = doc.createElement("spacer");
    spacerNode.setAttribute("flex", "10000");
    cell.appendChild(spacerNode);

    if (frameInfo.categoryData.label) {
      let categoryNode = doc.createElement("description");
      categoryNode.className = "plain call-tree-category";
      categoryNode.style.color = frameInfo.categoryData.color;
      categoryNode.setAttribute("value", frameInfo.categoryData.label);
      cell.appendChild(categoryNode);
    }
  },

  





  getDisplayedData: function() {
    if (this._cachedDisplayedData) {
      return this._cachedDisplayedData;
    }

    let data = this._cachedDisplayedData = Object.create(null);
    let frameInfo = this.frame.getInfo();

    















    
    let isLeaf = this._level === 0;
    let totalSamples = this.root.frame.samples;
    let totalDuration = this.root.frame.duration;

    
    if (this.visibleCells.selfDuration) {
      data.selfDuration = this.frame.youngestFrameSamples / totalSamples * totalDuration;
    }
    if (this.visibleCells.selfPercentage) {
      data.selfPercentage = this.frame.youngestFrameSamples / totalSamples * 100;
    }

    
    if (this.visibleCells.duration) {
      data.totalDuration = this.frame.samples / totalSamples * totalDuration;
    }
    if (this.visibleCells.percentage) {
      data.totalPercentage = this.frame.samples / totalSamples * 100;
    }

    
    if (this.visibleCells.samples) {
      data.samples = this.frame.youngestFrameSamples;
    }

    
    if (this.visibleCells.allocations) {
      let childrenAllocations = this.frame.calls.reduce((acc, node) => acc + node.allocations, 0);
      data.totalAllocations = this.frame.allocations + childrenAllocations;
    }
    if (this.visibleCells.selfAllocations) {
      data.selfAllocations = this.frame.allocations;
    }

    
    data.name = frameInfo.isMetaCategory
      ? frameInfo.categoryData.label
      : frameInfo.functionName || "";

    data.tooltiptext = frameInfo.isMetaCategory
      ? frameInfo.categoryData.label
      : this.frame.location || "";

    return this._cachedDisplayedData;
  },

  



  toggleCategories: function(visible) {
    if (!visible) {
      this.container.setAttribute("categories-hidden", "");
    } else {
      this.container.removeAttribute("categories-hidden");
    }
  },

  


  _onUrlClick: function(e) {
    e.preventDefault();
    e.stopPropagation();
    this.root.emit("link", this);
  },
});

exports.CallView = CallView;
