


"use strict";

const MIN_INSPECTOR_WIDTH = 300;


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
    this.el.setAttribute("width", Services.prefs.getIntPref("devtools.webaudioeditor.inspectorWidth"));
    this.button = $("#inspector-pane-toggle");
    mixin(this, ToggleMixin);
    this.bindToggle();

    
    this.hideImmediately();

    this._onNodeSelect = this._onNodeSelect.bind(this);
    this._onDestroyNode = this._onDestroyNode.bind(this);
    this._onResize = this._onResize.bind(this);
    this._onCommandClick = this._onCommandClick.bind(this);

    this.splitter.addEventListener("mouseup", this._onResize);
    for (let $el of $$("#audio-node-toolbar toolbarbutton")) {
      $el.addEventListener("command", this._onCommandClick);
    }
    window.on(EVENTS.UI_SELECT_NODE, this._onNodeSelect);
    gAudioNodes.on("remove", this._onDestroyNode);
  },

  


  destroy: function () {
    this.unbindToggle();
    this.splitter.removeEventListener("mouseup", this._onResize);

    $("#audio-node-toolbar toolbarbutton").removeEventListener("command", this._onCommandClick);
    for (let $el of $$("#audio-node-toolbar toolbarbutton")) {
      $el.removeEventListener("command", this._onCommandClick);
    }
    window.off(EVENTS.UI_SELECT_NODE, this._onNodeSelect);
    gAudioNodes.off("remove", this._onDestroyNode);

    this.el = null;
    this.button = null;
    this.splitter = null;
  },

  



  setCurrentAudioNode: Task.async(function* (node) {
    this._currentNode = node || null;

    
    
    if (!node) {
      $("#web-audio-editor-details-pane-empty").removeAttribute("hidden");
      $("#web-audio-editor-tabs").setAttribute("hidden", "true");
      window.emit(EVENTS.UI_INSPECTOR_NODE_SET, null);
    }
    
    else {
      $("#web-audio-editor-details-pane-empty").setAttribute("hidden", "true");
      $("#web-audio-editor-tabs").removeAttribute("hidden");
      yield this._buildToolbar();
      window.emit(EVENTS.UI_INSPECTOR_NODE_SET, this._currentNode.id);
    }
  }),

  


  getCurrentAudioNode: function () {
    return this._currentNode;
  },

  


  resetUI: function () {
    
    this.setCurrentAudioNode();

    
    this.hideImmediately();
  },

  _buildToolbar: Task.async(function* () {
    let node = this.getCurrentAudioNode();

    let bypassable = node.bypassable;
    let bypassed = yield node.isBypassed();
    let button = $("#audio-node-toolbar .bypass");

    if (!bypassable) {
      button.setAttribute("disabled", true);
    } else {
      button.removeAttribute("disabled");
    }

    if (!bypassable || bypassed) {
      button.removeAttribute("checked");
    } else {
      button.setAttribute("checked", true);
    }
  }),

  



  



  _onNodeSelect: function (_, id) {
    this.setCurrentAudioNode(gAudioNodes.get(id));

    
    this.show();
  },

  _onResize: function () {
    if (this.el.getAttribute("width") < MIN_INSPECTOR_WIDTH) {
      this.el.setAttribute("width", MIN_INSPECTOR_WIDTH);
    }
    Services.prefs.setIntPref("devtools.webaudioeditor.inspectorWidth", this.el.getAttribute("width"));
    window.emit(EVENTS.UI_INSPECTOR_RESIZE);
  },

  



  _onDestroyNode: function (node) {
    if (this._currentNode && this._currentNode.id === node.id) {
      this.setCurrentAudioNode(null);
    }
  },

  _onCommandClick: function (e) {
    let node = this.getCurrentAudioNode();
    let button = e.target;
    let command = button.getAttribute("data-command");
    let checked = button.getAttribute("checked");

    if (button.getAttribute("disabled")) {
      return;
    }

    if (command === "bypass") {
      if (checked) {
        button.removeAttribute("checked");
        node.bypass(true);
      } else {
        button.setAttribute("checked", true);
        node.bypass(false);
      }
    }
  }
};
