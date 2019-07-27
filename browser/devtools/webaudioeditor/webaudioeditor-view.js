


"use strict";

Cu.import("resource:///modules/devtools/VariablesView.jsm");
Cu.import("resource:///modules/devtools/VariablesViewController.jsm");
const { debounce } = require("sdk/lang/functional");


const EXPAND_INSPECTOR_STRING = L10N.getStr("expandInspector");
const COLLAPSE_INSPECTOR_STRING = L10N.getStr("collapseInspector");



const INSPECTOR_WIDTH = 300;



const GRAPH_DEFAULTS = {
  translate: [20, 20],
  scale: 1
};


const ARROW_HEIGHT = 5;
const ARROW_WIDTH = 8;


const MARKER_STYLING = {
  light: "#AAA",
  dark: "#CED3D9"
};

const GRAPH_DEBOUNCE_TIMER = 100;

const GENERIC_VARIABLES_VIEW_SETTINGS = {
  searchEnabled: false,
  editableValueTooltip: "",
  editableNameTooltip: "",
  preventDisableOnChange: true,
  preventDescriptorModifiers: false,
  eval: () => {}
};




let WebAudioGraphView = {
  


  initialize: function() {
    this._onGraphNodeClick = this._onGraphNodeClick.bind(this);
    this._onThemeChange = this._onThemeChange.bind(this);
    this._onNodeSelect = this._onNodeSelect.bind(this);
    this._onStartContext = this._onStartContext.bind(this);
    this._onDestroyNode = this._onDestroyNode.bind(this);

    this.draw = debounce(this.draw.bind(this), GRAPH_DEBOUNCE_TIMER);
    $('#graph-target').addEventListener('click', this._onGraphNodeClick, false);

    window.on(EVENTS.THEME_CHANGE, this._onThemeChange);
    window.on(EVENTS.UI_INSPECTOR_NODE_SET, this._onNodeSelect);
    window.on(EVENTS.START_CONTEXT, this._onStartContext);
    window.on(EVENTS.DESTROY_NODE, this._onDestroyNode);
  },

  


  destroy: function() {
    if (this._zoomBinding) {
      this._zoomBinding.on("zoom", null);
    }
    $('#graph-target').removeEventListener('click', this._onGraphNodeClick, false);
    window.off(EVENTS.THEME_CHANGE, this._onThemeChange);
    window.off(EVENTS.UI_INSPECTOR_NODE_SET, this._onNodeSelect);
    window.off(EVENTS.START_CONTEXT, this._onStartContext);
    window.off(EVENTS.DESTROY_NODE, this._onDestroyNode);
  },

  



  resetUI: function () {
    this.clearGraph();
    this.resetGraphPosition();
  },

  



  clearGraph: function () {
    $("#graph-target").innerHTML = "";
  },

  


  resetGraphPosition: function () {
    if (this._zoomBinding) {
      let { translate, scale } = GRAPH_DEFAULTS;
      
      
      this._zoomBinding.scale(scale);
      this._zoomBinding.translate(translate);
      d3.select("#graph-target")
        .attr("transform", "translate(" + translate + ") scale(" + scale + ")");
    }
  },

  getCurrentScale: function () {
    return this._zoomBinding ? this._zoomBinding.scale() : null;
  },

  getCurrentTranslation: function () {
    return this._zoomBinding ? this._zoomBinding.translate() : null;
  },

  





  focusNode: function (actorID) {
    
    Array.forEach($$(".nodes > g"), $node => $node.classList.remove("selected"));
    
    if (actorID) {
      this._getNodeByID(actorID).classList.add("selected");
    }
  },

  


  _getNodeByID: function (actorID) {
    return $(".nodes > g[data-id='" + actorID + "']");
  },

  





  draw: function () {
    
    this.clearGraph();

    let graph = new dagreD3.Digraph();
    
    
    
    let edges = [];

    AudioNodes.forEach(node => {
      
      graph.addNode(node.id, {
        type: node.type,                        
        label: node.type.replace(/Node$/, ""),  
        id: node.id                             
      });

      
      
      
      AudioNodeConnections.get(node, new Set()).forEach(dest => edges.push([node, dest]));
      let paramConnections = AudioParamConnections.get(node, {});
      Object.keys(paramConnections).forEach(destId => {
        let dest = getViewNodeById(destId);
        let connections = paramConnections[destId] || [];
        connections.forEach(param => edges.push([node, dest, param]));
      });
    });

    edges.forEach(([node, dest, param]) => {
      let options = {
        source: node.id,
        target: dest.id
      };

      
      
      
      if (param) {
        options.label = param;
        options.param = param;
      }

      graph.addEdge(null, node.id, dest.id, options);
    });

    let renderer = new dagreD3.Renderer();

    
    let oldDrawNodes = renderer.drawNodes();
    renderer.drawNodes(function(graph, root) {
      let svgNodes = oldDrawNodes(graph, root);
      svgNodes.attr("class", (n) => {
        let node = graph.node(n);
        return "audionode type-" + node.type;
      });
      svgNodes.attr("data-id", (n) => {
        let node = graph.node(n);
        return node.id;
      });
      return svgNodes;
    });

    
    
    
    
    let oldDrawEdgePaths = renderer.drawEdgePaths();
    renderer.drawEdgePaths(function(graph, root) {
      let svgEdges = oldDrawEdgePaths(graph, root);
      svgEdges.attr("data-source", (n) => {
        let edge = graph.edge(n);
        return edge.source;
      });
      svgEdges.attr("data-target", (n) => {
        let edge = graph.edge(n);
        return edge.target;
      });
      svgEdges.attr("data-param", (n) => {
        let edge = graph.edge(n);
        return edge.param ? edge.param : null;
      });
      
      
      let defaultClasses = "edgePath enter";
      svgEdges.attr("class", (n) => {
        let edge = graph.edge(n);
        return defaultClasses + (edge.param ? (" param-connection " + edge.param) : "");
      });

      return svgEdges;
    });

    
    
    renderer.postRender((graph, root) => {
      
      
      
      
      
      
      let theme = Services.prefs.getCharPref("devtools.theme");
      let markerColor = MARKER_STYLING[theme];
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
          .attr("style", "fill: " + markerColor)
          .append("svg:path")
          .attr("d", "M 0 0 L 10 5 L 0 10 z");
      }

      
      let currentNode = WebAudioInspectorView.getCurrentAudioNode();
      if (currentNode) {
        this.focusNode(currentNode.id);
      }

      
      let paramEdgeCount = edges.filter(p => !!p[2]).length;
      window.emit(EVENTS.UI_GRAPH_RENDERED, AudioNodes.length, edges.length - paramEdgeCount, paramEdgeCount);
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

      
      
      this.resetGraphPosition();
    }
  },

  



  



  _onStartContext: function () {
    this.draw();
  },

  


  _onDestroyNode: function () {
    this.draw();
  },

  _onNodeSelect: function (eventName, id) {
    this.focusNode(id);
  },

  


  _onThemeChange: function (eventName, theme) {
    let markerColor = MARKER_STYLING[theme];
    let marker = $("#arrowhead");
    if (marker) {
      marker.setAttribute("style", "fill: " + markerColor);
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
    this._onDestroyNode = this._onDestroyNode.bind(this);

    this._inspectorPaneToggleButton.addEventListener("mousedown", this._onTogglePaneClick, false);
    this._propsView = new VariablesView($("#properties-tabpanel-content"), GENERIC_VARIABLES_VIEW_SETTINGS);
    this._propsView.eval = this._onEval;

    window.on(EVENTS.UI_SELECT_NODE, this._onNodeSelect);
    window.on(EVENTS.DESTROY_NODE, this._onDestroyNode);
  },

  


  destroy: function () {
    this._inspectorPaneToggleButton.removeEventListener("mousedown", this._onTogglePaneClick);
    window.off(EVENTS.UI_SELECT_NODE, this._onNodeSelect);
    window.off(EVENTS.DESTROY_NODE, this._onDestroyNode);

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

  


  getCurrentAudioNode: function () {
    return this._currentNode;
  },

  


  resetUI: function () {
    this._propsView.empty();
    
    this.setCurrentAudioNode();

    
    this.toggleInspector({ visible: false, animated: false, delayed: false });
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
    this.setCurrentAudioNode(getViewNodeById(id));

    
    this.toggleInspector({ visible: true });
  },

  


  _onTogglePaneClick: function () {
    this.toggleInspector({ visible: !this.isVisible() });
  },

  



  _onDestroyNode: function (_, id) {
    if (this._currentNode && this._currentNode.id === id) {
      this.setCurrentAudioNode(null);
    }
  }
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
