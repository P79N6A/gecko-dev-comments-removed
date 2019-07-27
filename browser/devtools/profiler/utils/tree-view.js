


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");

loader.lazyRequireGetter(this, "L10N",
  "devtools/profiler/global", true);

loader.lazyImporter(this, "Heritage",
  "resource:///modules/devtools/ViewHelpers.jsm");
loader.lazyImporter(this, "AbstractTreeItem",
  "resource:///modules/devtools/AbstractTreeItem.jsm");

const URL_LABEL_TOOLTIP = L10N.getStr("table.url.tooltiptext");
const ZOOM_BUTTON_TOOLTIP = L10N.getStr("table.zoom.tooltiptext");
const CALL_TREE_INDENTATION = 16; 
const CALL_TREE_AUTO_EXPAND = 3; 

exports.CallView = CallView;























function CallView({ caller, frame, level }) {
  AbstractTreeItem.call(this, { parent: caller, level: level });

  this.autoExpandDepth = caller ? caller.autoExpandDepth : CALL_TREE_AUTO_EXPAND;
  this.frame = frame;

  this._onUrlClick = this._onUrlClick.bind(this);
  this._onZoomClick = this._onZoomClick.bind(this);
};

CallView.prototype = Heritage.extend(AbstractTreeItem.prototype, {
  





  _displaySelf: function(document, arrowNode) {
    this.document = document;

    let frameInfo = this.frame.getInfo();
    let framePercentage = this.frame.duration / this.root.frame.duration * 100;

    let durationCell = this._createTimeCell(this.frame.duration);
    let percentageCell = this._createExecutionCell(framePercentage);
    let samplesCell = this._createSamplesCell(this.frame.samples);
    let functionCell = this._createFunctionCell(arrowNode, frameInfo, this.level);

    let targetNode = document.createElement("hbox");
    targetNode.className = "call-tree-item";
    targetNode.setAttribute("origin", frameInfo.isContent ? "content" : "chrome");
    targetNode.setAttribute("category", frameInfo.categoryData.abbrev || "");
    targetNode.setAttribute("tooltiptext", this.frame.location || "");

    let isRoot = frameInfo.nodeType == "Thread";
    if (isRoot) {
      functionCell.querySelector(".call-tree-zoom").hidden = true;
      functionCell.querySelector(".call-tree-category").hidden = true;
    }

    targetNode.appendChild(durationCell);
    targetNode.appendChild(percentageCell);
    targetNode.appendChild(samplesCell);
    targetNode.appendChild(functionCell);

    return targetNode;
  },

  




  _populateSelf: function(children) {
    let newLevel = this.level + 1;

    for (let [, newFrame] of _Iterator(this.frame.calls)) {
      children.push(new CallView({
        caller: this,
        frame: newFrame,
        level: newLevel
      }));
    }

    
    children.sort((a, b) => a.frame.duration < b.frame.duration ? 1 : -1);
  },

  



  _createTimeCell: function(duration) {
    let cell = this.document.createElement("label");
    cell.className = "plain call-tree-cell";
    cell.setAttribute("type", "duration");
    cell.setAttribute("crop", "end");
    cell.setAttribute("value", L10N.numberWithDecimals(duration, 2));
    return cell;
  },
  _createExecutionCell: function(percentage) {
    let cell = this.document.createElement("label");
    cell.className = "plain call-tree-cell";
    cell.setAttribute("type", "percentage");
    cell.setAttribute("crop", "end");
    cell.setAttribute("value", L10N.numberWithDecimals(percentage, 2) + "%");
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
    nameNode.setAttribute("value", frameInfo.functionName || "");
    cell.appendChild(nameNode);

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

    let hostNode = this.document.createElement("label");
    hostNode.className = "plain call-tree-host";
    hostNode.setAttribute("value", frameInfo.hostName || "");
    cell.appendChild(hostNode);

    let zoomNode = this.document.createElement("button");
    zoomNode.className = "plain call-tree-zoom";
    zoomNode.setAttribute("tooltiptext", ZOOM_BUTTON_TOOLTIP);
    zoomNode.addEventListener("mousedown", this._onZoomClick);
    cell.appendChild(zoomNode);

    let spacerNode = this.document.createElement("spacer");
    spacerNode.setAttribute("flex", "10000");
    cell.appendChild(spacerNode);

    let categoryNode = this.document.createElement("label");
    categoryNode.className = "plain call-tree-category";
    categoryNode.style.color = frameInfo.categoryData.color;
    categoryNode.setAttribute("value", frameInfo.categoryData.label || "");
    cell.appendChild(categoryNode);

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
  },

  


  _onZoomClick: function(e) {
    e.preventDefault();
    e.stopPropagation();
    this.root.emit("zoom", this);
  }
});
