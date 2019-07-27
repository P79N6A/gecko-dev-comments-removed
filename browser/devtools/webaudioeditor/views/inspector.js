


"use strict";

Cu.import("resource:///modules/devtools/VariablesView.jsm");
Cu.import("resource:///modules/devtools/VariablesViewController.jsm");


const EXPAND_INSPECTOR_STRING = L10N.getStr("expandInspector");
const COLLAPSE_INSPECTOR_STRING = L10N.getStr("collapseInspector");



const INSPECTOR_WIDTH = 300;

const GENERIC_VARIABLES_VIEW_SETTINGS = {
  searchEnabled: false,
  editableValueTooltip: "",
  editableNameTooltip: "",
  preventDisableOnChange: true,
  preventDescriptorModifiers: false,
  eval: () => {}
};





let InspectorView = {
  _currentNode: null,

  
  _collapseString: COLLAPSE_INSPECTOR_STRING,
  _expandString: EXPAND_INSPECTOR_STRING,
  _toggleEvent: EVENTS.UI_INSPECTOR_TOGGLED,
  _animated: true,
  _delayed: true,

  


  initialize: function () {
    this._tabsPane = $("#web-audio-editor-tabs");

    
    this.el = $("#web-audio-inspector");
    this.el.setAttribute("width", INSPECTOR_WIDTH);
    this.button = $("#inspector-pane-toggle");
    mixin(this, ToggleMixin);
    this.bindToggle();

    
    this.hideImmediately();

    this._onEval = this._onEval.bind(this);
    this._onNodeSelect = this._onNodeSelect.bind(this);
    this._onDestroyNode = this._onDestroyNode.bind(this);

    this._propsView = new VariablesView($("#properties-tabpanel-content"), GENERIC_VARIABLES_VIEW_SETTINGS);
    this._propsView.eval = this._onEval;

    window.on(EVENTS.UI_SELECT_NODE, this._onNodeSelect);
    gAudioNodes.on("remove", this._onDestroyNode);
  },

  


  destroy: function () {
    this.unbindToggle();
    window.off(EVENTS.UI_SELECT_NODE, this._onNodeSelect);
    gAudioNodes.off("remove", this._onDestroyNode);

    this.el = null;
    this.button = null;
    this._tabsPane = null;
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
      this._buildPropertiesView()
        .then(() => window.emit(EVENTS.UI_INSPECTOR_NODE_SET, this._currentNode.id));
    }
  },

  


  getCurrentAudioNode: function () {
    return this._currentNode;
  },

  


  resetUI: function () {
    this._propsView.empty();
    
    this.setCurrentAudioNode();

    
    this.hideImmediately();
  },

  


  _setTitle: function () {
    let node = this._currentNode;
    let title = node.type.replace(/Node$/, "");
    $("#web-audio-inspector-title").setAttribute("value", title);
  },

  



  _buildPropertiesView: Task.async(function* () {
    let propsView = this._propsView;
    let node = this._currentNode;
    propsView.empty();

    let audioParamsScope = propsView.addScope("AudioParams");
    let props = yield node.getParams();

    
    
    this._togglePropertiesView(!!props.length);

    props.forEach(({ param, value, flags }) => {
      let descriptor = {
        value: value,
        writable: !flags || !flags.readonly,
      };
      audioParamsScope.addItem(param, descriptor);
    });

    audioParamsScope.expanded = true;

    window.emit(EVENTS.UI_PROPERTIES_TAB_RENDERED, node.id);
  }),

  _togglePropertiesView: function (show) {
    let propsView = $("#properties-tabpanel-content");
    let emptyView = $("#properties-tabpanel-content-empty");
    (show ? propsView : emptyView).removeAttribute("hidden");
    (show ? emptyView : propsView).setAttribute("hidden", "true");
  },

  





  _getAudioPropertiesScope: function () {
    return this._propsView.getScopeAtIndex(0);
  },

  



  


  _onEval: Task.async(function* (variable, value) {
    let ownerScope = variable.ownerView;
    let node = this._currentNode;
    let propName = variable.name;
    let error;

    if (!variable._initialDescriptor.writable) {
      error = new Error("Variable " + propName + " is not writable.");
    } else {
      
      try {
        let number = parseFloat(value);
        if (!isNaN(number)) {
          value = number;
        } else {
          value = JSON.parse(value);
        }
        error = yield node.actor.setParam(propName, value);
      }
      catch (e) {
        error = e;
      }
    }

    
    
    
    if (!error) {
      ownerScope.get(propName).setGrip(value);
      window.emit(EVENTS.UI_SET_PARAM, node.id, propName, value);
    } else {
      window.emit(EVENTS.UI_SET_PARAM_ERROR, node.id, propName, value);
    }
  }),

  



  _onNodeSelect: function (_, id) {
    this.setCurrentAudioNode(gAudioNodes.get(id));

    
    this.show();
  },

  



  _onDestroyNode: function (node) {
    if (this._currentNode && this._currentNode.id === node.id) {
      this.setCurrentAudioNode(null);
    }
  }
};
