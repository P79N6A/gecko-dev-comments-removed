


"use strict";

Cu.import("resource:///modules/devtools/VariablesView.jsm");
Cu.import("resource:///modules/devtools/VariablesViewController.jsm");

const GENERIC_VARIABLES_VIEW_SETTINGS = {
  searchEnabled: false,
  editableValueTooltip: "",
  editableNameTooltip: "",
  preventDisableOnChange: true,
  preventDescriptorModifiers: false,
  eval: () => {}
};





let PropertiesView = {

  


  initialize: function () {
    this._onEval = this._onEval.bind(this);
    this._onNodeSet = this._onNodeSet.bind(this);

    window.on(EVENTS.UI_INSPECTOR_NODE_SET, this._onNodeSet);
    this._propsView = new VariablesView($("#properties-content"), GENERIC_VARIABLES_VIEW_SETTINGS);
    this._propsView.eval = this._onEval;
  },

  


  destroy: function () {
    window.off(EVENTS.UI_INSPECTOR_NODE_SET, this._onNodeSet);
    this._propsView = null;
  },

  


  resetUI: function () {
    this._propsView.empty();
    this._currentNode = null;
  },

  



  _setAudioNode: function (node) {
    this._currentNode = node;
    if (this._currentNode) {
      this._buildPropertiesView();
    }
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
      let item = audioParamsScope.addItem(param, descriptor);

      
      item.twisty = false;
    });

    audioParamsScope.expanded = true;

    window.emit(EVENTS.UI_PROPERTIES_TAB_RENDERED, node.id);
  }),

  



  _togglePropertiesView: function (show) {
    let propsView = $("#properties-content");
    let emptyView = $("#properties-empty");
    (show ? propsView : emptyView).removeAttribute("hidden");
    (show ? emptyView : propsView).setAttribute("hidden", "true");
  },

  





  _getAudioPropertiesScope: function () {
    return this._propsView.getScopeAtIndex(0);
  },

  



  


  _onNodeSet: function (_, id) {
    this._setAudioNode(gAudioNodes.get(id));
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
  })
};
