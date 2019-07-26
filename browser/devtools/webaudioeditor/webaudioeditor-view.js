


"use strict";

Cu.import("resource:///modules/devtools/VariablesView.jsm");
Cu.import("resource:///modules/devtools/VariablesViewController.jsm");
const { debounce } = require("sdk/lang/functional");


const EXPAND_INSPECTOR_STRING = L10N.getStr("expandInspector");
const COLLAPSE_INSPECTOR_STRING = L10N.getStr("collapseInspector");



const INSPECTOR_WIDTH = 300;




const WIDTH = 1000;
const HEIGHT = 400;


const ARROW_HEIGHT = 5;
const ARROW_WIDTH = 8;

const GRAPH_DEBOUNCE_TIMER = 100;

const GENERIC_VARIABLES_VIEW_SETTINGS = {
  searchEnabled: false,
  editableValueTooltip: "",
  editableNameTooltip: "",
  preventDisableOnChange: true,
  preventDescriptorModifiers: true,
  eval: () => {}
};




let WebAudioGraphView = {
  


  initialize: function() {
    this._onGraphNodeClick = this._onGraphNodeClick.bind(this);
    this.draw = debounce(this.draw.bind(this), GRAPH_DEBOUNCE_TIMER);
    $('#graph-target').addEventListener('click', this._onGraphNodeClick, false);
  },

  


  destroy: function() {
    if (this._zoomBinding) {
      this._zoomBinding.on("zoom", null);
    }
    $('#graph-target').removeEventListener('click', this._onGraphNodeClick, false);
  },

  



  resetUI: function () {
    $("#reload-notice").hidden = true;
    $("#waiting-notice").hidden = false;
    $("#content").hidden = true;
    this.resetGraph();
  },

  



  showContent: function () {
    $("#reload-notice").hidden = true;
    $("#waiting-notice").hidden = true;
    $("#content").hidden = false;
    this.draw();
  },

  



  resetGraph: function () {
    $("#graph-target").innerHTML = "";
  },

  


  focusNode: function (actorID) {
    
    Array.prototype.forEach.call($$(".nodes > g"), $node => $node.classList.remove("selected"));
    
    this._getNodeByID(actorID).classList.add("selected");
  },

  


  blurNode: function (actorID) {
    this._getNodeByID(actorID).classList.remove("selected");
  },

  


  _getNodeByID: function (actorID) {
    return $(".nodes > g[data-id='" + actorID + "']");
  },

  




  draw: function () {
    
    this.resetGraph();

    let graph = new dagreD3.Digraph();
    let edges = [];

    AudioNodes.forEach(node => {
      
      graph.addNode(node.id, { label: node.type, id: node.id });

      
      
      
      AudioNodeConnections.get(node, []).forEach(dest => edges.push([node, dest]));
    });

    edges.forEach(([node, dest]) => graph.addEdge(null, node.id, dest.id, {
      source: node.id,
      target: dest.id
    }));

    let renderer = new dagreD3.Renderer();

    
    let oldDrawNodes = renderer.drawNodes();
    renderer.drawNodes(function(graph, root) {
      let svgNodes = oldDrawNodes(graph, root);
      svgNodes.attr("class", (n) => {
        let node = graph.node(n);
        return "audionode type-" + node.label;
      });
      svgNodes.attr("data-id", (n) => {
        let node = graph.node(n);
        return node.id;
      });
      return svgNodes;
    });

    
    let oldDrawEdgePaths = renderer.drawEdgePaths();
    renderer.drawEdgePaths(function(graph, root) {
      let svgNodes = oldDrawEdgePaths(graph, root);
      svgNodes.attr("data-source", (n) => {
        let edge = graph.edge(n);
        return edge.source;
      });
      svgNodes.attr("data-target", (n) => {
        let edge = graph.edge(n);
        return edge.target;
      });
      return svgNodes;
    });

    
    
    renderer.postRender(function (graph, root) {
      
      
      
      
      if (graph.isDirected() && root.select("#arrowhead").empty()) {
        root
          .append("svg:defs")
          .append("svg:marker")
          .attr("id", "arrowhead")
          .attr("viewBox", "0 0 10 10")
          .attr("refX", ARROW_WIDTH)
          .attr("refY", ARROW_HEIGHT)
          .attr("markerUnits", "strokewidth")
          .attr("markerWidth", ARROW_WIDTH)
          .attr("markerHeight", ARROW_HEIGHT)
          .attr("orient", "auto")
          .attr("style", "fill: #f5f7fa")
          .append("svg:path")
          .attr("d", "M 0 0 L 10 5 L 0 10 z");
      }

      
      window.emit(EVENTS.UI_GRAPH_RENDERED, AudioNodes.length, edges.length);
    });

    let layout = dagreD3.layout().rankDir("LR");
    renderer.layout(layout).run(graph, d3.select("#graph-target"));

    
    
    if (!this._zoomBinding) {
      this._zoomBinding = d3.behavior.zoom().on("zoom", function () {
        var ev = d3.event;
        d3.select("#graph-target")
          .attr("transform", "translate(" + ev.translate + ") scale(" + ev.scale + ")");
      });
      d3.select("svg").call(this._zoomBinding);
    }
  },

  



  





  _onGraphNodeClick: function (e) {
    let node = findGraphNodeParent(e.target);
    
    
    if (!node)
      return;

    window.emit(EVENTS.UI_SELECT_NODE, node.getAttribute("data-id"));
  }
};

let WebAudioInspectorView = {

  _propsView: null,

  _currentNode: null,

  _inspectorPane: null,
  _inspectorPaneToggleButton: null,
  _tabsPane: null,

  


  initialize: function () {
    this._inspectorPane = $("#web-audio-inspector");
    this._inspectorPaneToggleButton = $("#inspector-pane-toggle");
    this._tabsPane = $("#web-audio-editor-tabs");

    
    this._inspectorPane.setAttribute("width", INSPECTOR_WIDTH);
    this.toggleInspector({ visible: false, delayed: false, animated: false });

    this._onEval = this._onEval.bind(this);
    this._onNodeSelect = this._onNodeSelect.bind(this);
    this._onTogglePaneClick = this._onTogglePaneClick.bind(this);

    this._inspectorPaneToggleButton.addEventListener("mousedown", this._onTogglePaneClick, false);
    this._propsView = new VariablesView($("#properties-tabpanel-content"), GENERIC_VARIABLES_VIEW_SETTINGS);
    this._propsView.eval = this._onEval;

    window.on(EVENTS.UI_SELECT_NODE, this._onNodeSelect);
  },

  


  destroy: function () {
    this._inspectorPaneToggleButton.removeEventListener("mousedown", this._onTogglePaneClick);
    window.off(EVENTS.UI_SELECT_NODE, this._onNodeSelect);

    this._inspectorPane = null;
    this._inspectorPaneToggleButton = null;
    this._tabsPane = null;
  },

  











  toggleInspector: function ({ visible, animated, delayed, index }) {
    let pane = this._inspectorPane;
    let button = this._inspectorPaneToggleButton;

    let flags = {
      visible: visible,
      animated: animated != null ? animated : true,
      delayed: delayed != null ? delayed : true,
      callback: () => window.emit(EVENTS.UI_INSPECTOR_TOGGLED, visible)
    };

    ViewHelpers.togglePane(flags, pane);

    if (flags.visible) {
      button.removeAttribute("pane-collapsed");
      button.setAttribute("tooltiptext", COLLAPSE_INSPECTOR_STRING);
    }
    else {
      button.setAttribute("pane-collapsed", "");
      button.setAttribute("tooltiptext", EXPAND_INSPECTOR_STRING);
    }

    if (index != undefined) {
      pane.selectedIndex = index;
    }
  },

  



  isVisible: function () {
    return !this._inspectorPane.hasAttribute("pane-collapsed");
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

  


  getCurrentNode: function () {
    return this._currentNode;
  },

  


  resetUI: function () {
    this._propsView.empty();
    
    this.setCurrentAudioNode();

    
    this.toggleInspector({ visible: false, animated: false, delayed: false });
  },

  


  _setTitle: function () {
    let node = this._currentNode;
    let title = node.type + " (" + node.id + ")";
    $("#web-audio-inspector-title").setAttribute("value", title);
  },

  



  _buildPropertiesView: Task.async(function* () {
    let propsView = this._propsView;
    let node = this._currentNode;
    propsView.empty();

    let audioParamsScope = propsView.addScope("AudioParams");
    let props = yield node.getParams();

    
    
    this._togglePropertiesView(!!props.length);

    props.forEach(({ param, value }) => {
      let descriptor = { value: value };
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

    
    try {
      value = JSON.parse(value);
      error = yield node.actor.setParam(propName, value);
    }
    catch (e) {
      error = e;
    }

    
    
    
    if (!error) {
      ownerScope.get(propName).setGrip(value);
      window.emit(EVENTS.UI_SET_PARAM, node.id, propName, value);
    } else {
      window.emit(EVENTS.UI_SET_PARAM_ERROR, node.id, propName, value);
    }
  }),

  



  _onNodeSelect: function (_, id) {
    this.setCurrentAudioNode(getViewNodeById(id));

    
    this.toggleInspector({ visible: true });
  },

  


  _onTogglePaneClick: function () {
    this.toggleInspector({ visible: !this.isVisible() });
  },

  



  removeNode: Task.async(function* (viewNode) {

  })
};







function findGraphNodeParent (el) {
  
  if (!el.classList)
    return null;

  while (!el.classList.contains("nodes")) {
    if (el.classList.contains("audionode"))
      return el;
    else
      el = el.parentNode;
  }
  return null;
}
