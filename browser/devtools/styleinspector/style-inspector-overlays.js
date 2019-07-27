





"use strict";







const {Cc, Ci, Cu} = require("chrome");
const {
  Tooltip,
  SwatchColorPickerTooltip,
  SwatchCubicBezierTooltip,
  SwatchFilterTooltip
} = require("devtools/shared/widgets/Tooltip");
const {CssLogic} = require("devtools/styleinspector/css-logic");
const {Promise:promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const PREF_IMAGE_TOOLTIP_SIZE = "devtools.inspector.imagePreviewTooltipSize";


const TOOLTIP_IMAGE_TYPE = "image";
const TOOLTIP_FONTFAMILY_TYPE = "font-family";


const VIEW_NODE_SELECTOR_TYPE = exports.VIEW_NODE_SELECTOR_TYPE = 1;
const VIEW_NODE_PROPERTY_TYPE = exports.VIEW_NODE_PROPERTY_TYPE = 2;
const VIEW_NODE_VALUE_TYPE = exports.VIEW_NODE_VALUE_TYPE = 3;
const VIEW_NODE_IMAGE_URL_TYPE = exports.VIEW_NODE_IMAGE_URL_TYPE = 4;






function HighlightersOverlay(view) {
  this.view = view;

  let {CssRuleView} = require("devtools/styleinspector/rule-view");
  this.isRuleView = view instanceof CssRuleView;

  this.highlighterUtils = this.view.inspector.toolbox.highlighterUtils;

  this._onMouseMove = this._onMouseMove.bind(this);
  this._onMouseLeave = this._onMouseLeave.bind(this);

  this.promises = {};
  this.highlighters = {};

  
  
  this.supportsHighlighters = this.highlighterUtils.supportsCustomHighlighters();
}

exports.HighlightersOverlay = HighlightersOverlay;

HighlightersOverlay.prototype = {
  



  addToView: function() {
    if (!this.supportsHighlighters || this._isStarted || this._isDestroyed) {
      return;
    }

    let el = this.view.element;
    el.addEventListener("mousemove", this._onMouseMove, false);
    el.addEventListener("mouseleave", this._onMouseLeave, false);

    this._isStarted = true;
  },

  



  removeFromView: function() {
    if (!this.supportsHighlighters || !this._isStarted || this._isDestroyed) {
      return;
    }

    this._hideCurrent();

    let el = this.view.element;
    el.removeEventListener("mousemove", this._onMouseMove, false);
    el.removeEventListener("mouseleave", this._onMouseLeave, false);

    this._isStarted = false;
  },

  _onMouseMove: function(event) {
    
    if (event.target === this._lastHovered) {
      return;
    }

    
    this._hideCurrent();

    this._lastHovered = event.target;

    let nodeInfo = this.view.getNodeInfo(event.target);
    if (!nodeInfo) {
      return;
    }

    
    let type;
    if (this._isRuleViewTransform(nodeInfo) ||
        this._isComputedViewTransform(nodeInfo)) {
      type = "CssTransformHighlighter";
    }

    if (type) {
      this.highlighterShown = type;
      let node = this.view.inspector.selection.nodeFront;
      this._getHighlighter(type).then(highlighter => {
        highlighter.show(node);
      });
    }
  },

  _onMouseLeave: function(event) {
    this._lastHovered = null;
    this._hideCurrent();
  },

  




  _isRuleViewTransform: function(nodeInfo) {
    let isTransform = nodeInfo.type === VIEW_NODE_VALUE_TYPE &&
                      nodeInfo.value.property === "transform";
    let isEnabled = nodeInfo.value.enabled &&
                    !nodeInfo.value.overridden &&
                    !nodeInfo.value.pseudoElement;
    return this.isRuleView && isTransform && isEnabled;
  },

  





  _isComputedViewTransform: function(nodeInfo) {
    let isTransform = nodeInfo.type === VIEW_NODE_VALUE_TYPE &&
                      nodeInfo.value.property === "transform";
    return !this.isRuleView && isTransform;
  },

  


  _hideCurrent: function() {
    if (this.highlighterShown) {
      this._getHighlighter(this.highlighterShown).then(highlighter => {
        
        
        
        
        let promise = highlighter.hide();
        if (promise) {
          promise.then(null, Cu.reportError);
        }
        this.highlighterShown = null;
      });
    }
  },

  




  _getHighlighter: function(type) {
    let utils = this.highlighterUtils;

    if (this.promises[type]) {
      return this.promises[type];
    }

    return this.promises[type] = utils.getHighlighterByType(type).then(highlighter => {
      this.highlighters[type] = highlighter;
      return highlighter;
    });
  },

  



  destroy: function() {
    this.removeFromView();

    for (let type in this.highlighters) {
      if (this.highlighters[type]) {
        this.highlighters[type].finalize();
        this.highlighters[type] = null;
      }
    }

    this.promises = null;
    this.view = null;
    this.highlighterUtils = null;

    this._isDestroyed = true;
  }
};






function TooltipsOverlay(view) {
  this.view = view;

  let {CssRuleView} = require("devtools/styleinspector/rule-view");
  this.isRuleView = view instanceof CssRuleView;

  this._onNewSelection = this._onNewSelection.bind(this);
  this.view.inspector.selection.on("new-node-front", this._onNewSelection);
}

exports.TooltipsOverlay = TooltipsOverlay;

TooltipsOverlay.prototype = {
  get isEditing() {
    return this.colorPicker.tooltip.isShown() ||
           this.colorPicker.eyedropperOpen ||
           this.cubicBezier.tooltip.isShown() ||
           this.filterEditor.tooltip.isShown();
  },

  



  addToView: function() {
    if (this._isStarted || this._isDestroyed) {
      return;
    }

    
    this.previewTooltip = new Tooltip(this.view.inspector.panelDoc);
    this.previewTooltip.startTogglingOnHover(this.view.element,
      this._onPreviewTooltipTargetHover.bind(this));

    if (this.isRuleView) {
      
      this.colorPicker = new SwatchColorPickerTooltip(this.view.inspector.panelDoc);
      
      this.cubicBezier = new SwatchCubicBezierTooltip(this.view.inspector.panelDoc);
      
      this.filterEditor = new SwatchFilterTooltip(this.view.inspector.panelDoc);
    }

    this._isStarted = true;
  },

  



  removeFromView: function() {
    if (!this._isStarted || this._isDestroyed) {
      return;
    }

    this.previewTooltip.stopTogglingOnHover(this.view.element);
    this.previewTooltip.destroy();

    if (this.colorPicker) {
      this.colorPicker.destroy();
    }

    if (this.cubicBezier) {
      this.cubicBezier.destroy();
    }

    if (this.filterEditor) {
      this.filterEditor.destroy();
    }

    this._isStarted = false;
  },

  





  _getTooltipType: function({type, value:prop}) {
    let tooltipType = null;
    let inspector = this.view.inspector;

    
    if (type === VIEW_NODE_IMAGE_URL_TYPE && inspector.hasUrlToImageDataResolver) {
      tooltipType = TOOLTIP_IMAGE_TYPE;
    }

    
    if (type === VIEW_NODE_VALUE_TYPE && prop.property === "font-family") {
      let value = prop.value.toLowerCase();
      if (value !== "inherit" && value !== "unset" && value !== "initial") {
        tooltipType = TOOLTIP_FONTFAMILY_TYPE;
      }
    }

    return tooltipType;
  },

  






  _onPreviewTooltipTargetHover: function(target) {
    let nodeInfo = this.view.getNodeInfo(target);
    if (!nodeInfo) {
      
      return promise.reject();
    }

    let type = this._getTooltipType(nodeInfo);
    if (!type) {
      
      return promise.reject();
    }

    if (this.isRuleView && this.colorPicker.tooltip.isShown()) {
      this.colorPicker.revert();
      this.colorPicker.hide();
    }

    if (this.isRuleView && this.cubicBezier.tooltip.isShown()) {
      this.cubicBezier.revert();
      this.cubicBezier.hide();
    }

    if (this.isRuleView && this.filterEditor.tooltip.isShown()) {
      this.filterEditor.revert();
      this.filterEdtior.hide();
    }

    let inspector = this.view.inspector;

    if (type === TOOLTIP_IMAGE_TYPE) {
      let dim = Services.prefs.getIntPref(PREF_IMAGE_TOOLTIP_SIZE);
      
      let uri = nodeInfo.value.url;
      return this.previewTooltip.setRelativeImageContent(uri,
        inspector.inspector, dim);
    }

    if (type === TOOLTIP_FONTFAMILY_TYPE) {
      return this.previewTooltip.setFontFamilyContent(nodeInfo.value.value,
        inspector.selection.nodeFront);
    }
  },

  _onNewSelection: function() {
    if (this.previewTooltip) {
      this.previewTooltip.hide();
    }

    if (this.colorPicker) {
      this.colorPicker.hide();
    }

    if (this.cubicBezier) {
      this.cubicBezier.hide();
    }

    if (this.filterEditor) {
      this.filterEditor.hide();
    }
  },

  


  destroy: function() {
    this.removeFromView();

    this.view.inspector.selection.off("new-node-front", this._onNewSelection);
    this.view = null;

    this._isDestroyed = true;
  }
};
