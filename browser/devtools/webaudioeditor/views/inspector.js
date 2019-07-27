


"use strict";



const INSPECTOR_WIDTH = 300;


const EXPAND_INSPECTOR_STRING = L10N.getStr("expandInspector");
const COLLAPSE_INSPECTOR_STRING = L10N.getStr("collapseInspector");





let InspectorView = {
  _currentNode: null,

  
  _collapseString: COLLAPSE_INSPECTOR_STRING,
  _expandString: EXPAND_INSPECTOR_STRING,
  _toggleEvent: EVENTS.UI_INSPECTOR_TOGGLED,
  _animated: true,
  _delayed: true,

  


  initialize: function () {
    
    this.el = $("#web-audio-inspector");
    this.splitter = $("#inspector-splitter");
    this.el.setAttribute("width", INSPECTOR_WIDTH);
    this.button = $("#inspector-pane-toggle");
    mixin(this, ToggleMixin);
    this.bindToggle();

    
    this.hideImmediately();

    this._onNodeSelect = this._onNodeSelect.bind(this);
    this._onDestroyNode = this._onDestroyNode.bind(this);
    this._onResize = this._onResize.bind(this);

    this.splitter.addEventListener("mouseup", this._onResize);
    window.on(EVENTS.UI_SELECT_NODE, this._onNodeSelect);
    gAudioNodes.on("remove", this._onDestroyNode);
  },

  


  destroy: function () {
    this.unbindToggle();
    this.splitter.removeEventListener("mouseup", this._onResize);
    window.off(EVENTS.UI_SELECT_NODE, this._onNodeSelect);
    gAudioNodes.off("remove", this._onDestroyNode);

    this.el = null;
    this.button = null;
    this.splitter = null;
  },

  



  setCurrentAudioNode: function (node) {
    this._currentNode = node || null;

    
    
    if (!node) {
      $("#web-audio-editor-details-pane-empty").removeAttribute("hidden");
      $("#web-audio-editor-tabs").setAttribute("hidden", "true");
      window.emit(EVENTS.UI_INSPECTOR_NODE_SET, null);
    }
    
    else {
      $("#web-audio-editor-details-pane-empty").setAttribute("hidden", "true");
      $("#web-audio-editor-tabs").removeAttribute("hidden");
      this._setTitle();
      window.emit(EVENTS.UI_INSPECTOR_NODE_SET, this._currentNode.id);
    }
  },

  


  getCurrentAudioNode: function () {
    return this._currentNode;
  },

  


  resetUI: function () {
    
    this.setCurrentAudioNode();

    
    this.hideImmediately();
  },

  


  _setTitle: function () {
    let node = this._currentNode;
    let title = node.type.replace(/Node$/, "");
    $("#web-audio-inspector-title").setAttribute("value", title);
  },

  



  



  _onNodeSelect: function (_, id) {
    this.setCurrentAudioNode(gAudioNodes.get(id));

    
    this.show();
  },

  _onResize: function () {
    window.emit(EVENTS.UI_INSPECTOR_RESIZE);
  },

  



  _onDestroyNode: function (node) {
    if (this._currentNode && this._currentNode.id === node.id) {
      this.setCurrentAudioNode(null);
    }
  }
};
