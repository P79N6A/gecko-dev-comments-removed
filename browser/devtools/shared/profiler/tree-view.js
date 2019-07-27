


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");

loader.lazyRequireGetter(this, "L10N",
  "devtools/shared/profiler/global", true);

loader.lazyImporter(this, "Heritage",
  "resource:///modules/devtools/ViewHelpers.jsm");
loader.lazyImporter(this, "AbstractTreeItem",
  "resource:///modules/devtools/AbstractTreeItem.jsm");

const MILLISECOND_UNITS = L10N.getStr("table.ms");
const PERCENTAGE_UNITS = L10N.getStr("table.percentage");
const URL_LABEL_TOOLTIP = L10N.getStr("table.url.tooltiptext");
const CALL_TREE_INDENTATION = 16; 

const DEFAULT_SORTING_PREDICATE = (a, b) => a.frame.samples < b.frame.samples ? 1 : -1;
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

exports.CallView = CallView;









































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
    this.document = document;

    let frameInfo = this.frame.getInfo();
    let framePercentage = this._getPercentage(this.frame.samples);

    let selfPercentage;
    let selfDuration;
    let totalAllocations;

    let frameKey = this.frame.key;
    if (this.visibleCells.selfPercentage) {
      selfPercentage = this._getPercentage(this.root.frame.selfCount[frameKey]);
    }
    if (this.visibleCells.selfDuration) {
      selfDuration = this.root.frame.selfDuration[frameKey];
    }

    if (!this.frame.calls.length) {
      if (this.visibleCells.allocations) {
        totalAllocations = this.frame.allocations;
      }
    } else {
      if (this.visibleCells.allocations) {
        let childrenAllocations = this.frame.calls.reduce((acc, node) => acc + node.allocations, 0);
        totalAllocations = this.frame.allocations + childrenAllocations;
      }
    }

    if (this.visibleCells.duration) {
      var durationCell = this._createTimeCell(this.frame.duration);
    }
    if (this.visibleCells.selfDuration) {
      var selfDurationCell = this._createTimeCell(selfDuration, true);
    }
    if (this.visibleCells.percentage) {
      var percentageCell = this._createExecutionCell(framePercentage);
    }
    if (this.visibleCells.selfPercentage) {
      var selfPercentageCell = this._createExecutionCell(selfPercentage, true);
    }
    if (this.visibleCells.allocations) {
      var allocationsCell = this._createAllocationsCell(totalAllocations);
    }
    if (this.visibleCells.selfAllocations) {
      var selfAllocationsCell = this._createAllocationsCell(this.frame.allocations, true);
    }
    if (this.visibleCells.samples) {
      var samplesCell = this._createSamplesCell(this.frame.samples);
    }
    if (this.visibleCells.function) {
      var functionCell = this._createFunctionCell(arrowNode, frameInfo, this.level);
    }

    let targetNode = document.createElement("hbox");
    targetNode.className = "call-tree-item";
    targetNode.setAttribute("origin", frameInfo.isContent ? "content" : "chrome");
    targetNode.setAttribute("category", frameInfo.categoryData.abbrev || "");
    targetNode.setAttribute("tooltiptext", frameInfo.isMetaCategory ? frameInfo.categoryData.label :
                                           this.frame.location || "");
    if (this.hidden) {
      targetNode.style.display = "none";
    }

    let isRoot = frameInfo.nodeType == "Thread";
    if (isRoot) {
      functionCell.querySelector(".call-tree-category").hidden = true;
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

  


  _getPercentage: function(samples) {
    return samples / this.root.frame.samples * 100;
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

    
    
    children.sort(this.sortingPredicate);
  },

  



  _createTimeCell: function(duration, isSelf = false) {
    let cell = this.document.createElement("label");
    cell.className = "plain call-tree-cell";
    cell.setAttribute("type", isSelf ? "self-duration" : "duration");
    cell.setAttribute("crop", "end");
    cell.setAttribute("value", L10N.numberWithDecimals(duration, 2) + " " + MILLISECOND_UNITS);
    return cell;
  },
  _createExecutionCell: function(percentage, isSelf = false) {
    let cell = this.document.createElement("label");
    cell.className = "plain call-tree-cell";
    cell.setAttribute("type", isSelf ? "self-percentage" : "percentage");
    cell.setAttribute("crop", "end");
    cell.setAttribute("value", L10N.numberWithDecimals(percentage, 2) + PERCENTAGE_UNITS);
    return cell;
  },
  _createAllocationsCell: function(count, isSelf = false) {
    let cell = this.document.createElement("label");
    cell.className = "plain call-tree-cell";
    cell.setAttribute("type", isSelf ? "self-allocations" : "allocations");
    cell.setAttribute("crop", "end");
    cell.setAttribute("value", count || 0);
    return cell;
  },
  _createSamplesCell: function(count) {
    let cell = this.document.createElement("label");
    cell.className = "plain call-tree-cell";
    cell.setAttribute("type", "samples");
    cell.setAttribute("crop", "end");
    cell.setAttribute("value", count || "");
    return cell;
  },
  _createFunctionCell: function(arrowNode, frameInfo, frameLevel) {
    let cell = this.document.createElement("hbox");
    cell.className = "call-tree-cell";
    cell.style.MozMarginStart = (frameLevel * CALL_TREE_INDENTATION) + "px";
    cell.setAttribute("type", "function");
    cell.appendChild(arrowNode);

    let nameNode = this.document.createElement("label");
    nameNode.className = "plain call-tree-name";
    nameNode.setAttribute("flex", "1");
    nameNode.setAttribute("crop", "end");
    nameNode.setAttribute("value", frameInfo.isMetaCategory
                                     ? frameInfo.categoryData.label
                                     : frameInfo.functionName || "");
    cell.appendChild(nameNode);

    
    if (!frameInfo.isMetaCategory) {
      let urlNode = this.document.createElement("label");
      urlNode.className = "plain call-tree-url";
      urlNode.setAttribute("flex", "1");
      urlNode.setAttribute("crop", "end");
      urlNode.setAttribute("value", frameInfo.fileName || "");
      urlNode.setAttribute("tooltiptext", URL_LABEL_TOOLTIP + " â†’ " + frameInfo.url);
      urlNode.addEventListener("mousedown", this._onUrlClick);
      cell.appendChild(urlNode);

      let lineNode = this.document.createElement("label");
      lineNode.className = "plain call-tree-line";
      lineNode.setAttribute("value", frameInfo.line ? ":" + frameInfo.line : "");
      cell.appendChild(lineNode);

      let columnNode = this.document.createElement("label");
      columnNode.className = "plain call-tree-column";
      columnNode.setAttribute("value", frameInfo.column ? ":" + frameInfo.column : "");
      cell.appendChild(columnNode);

      let hostNode = this.document.createElement("label");
      hostNode.className = "plain call-tree-host";
      hostNode.setAttribute("value", frameInfo.host || "");
      cell.appendChild(hostNode);

      let spacerNode = this.document.createElement("spacer");
      spacerNode.setAttribute("flex", "10000");
      cell.appendChild(spacerNode);

      let categoryNode = this.document.createElement("label");
      categoryNode.className = "plain call-tree-category";
      categoryNode.style.color = frameInfo.categoryData.color;
      categoryNode.setAttribute("value", frameInfo.categoryData.label || "");
      cell.appendChild(categoryNode);
    }

    let hasDescendants = Object.keys(this.frame.calls).length > 0;
    if (hasDescendants == false) {
      arrowNode.setAttribute("invisible", "");
    }

    return cell;
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
  }
});
