


"use strict";

Cu.import("resource:///modules/devtools/VariablesView.jsm");
Cu.import("resource:///modules/devtools/VariablesViewController.jsm");
const { debounce } = require("sdk/lang/functional");




const WIDTH = 1000;
const HEIGHT = 400;


const ARROW_HEIGHT = 5;
const ARROW_WIDTH = 8;

const GRAPH_DEBOUNCE_TIMER = 100;

const GENERIC_VARIABLES_VIEW_SETTINGS = {
  lazyEmpty: true,
  lazyEmptyDelay: 10, 
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
    WebAudioParamView.focusNode(node.getAttribute('data-id'));
  }
};

let WebAudioParamView = {
  _paramsView: null,

  


  initialize: function () {
    this._paramsView = new VariablesView($("#web-audio-inspector-content"), GENERIC_VARIABLES_VIEW_SETTINGS);
    this._paramsView.eval = this._onEval.bind(this);
    window.on(EVENTS.CREATE_NODE, this.addNode = this.addNode.bind(this));
    window.on(EVENTS.DESTROY_NODE, this.removeNode = this.removeNode.bind(this));
  },

  


  destroy: function() {
    window.off(EVENTS.CREATE_NODE, this.addNode);
    window.off(EVENTS.DESTROY_NODE, this.removeNode);
  },

  


  resetUI: function () {
    this._paramsView.empty();
  },

  


  focusNode: function (id) {
    let scope = this._getScopeByID(id);
    if (!scope) return;

    scope.focus();
    scope.expand();
  },

  


  _onEval: Task.async(function* (variable, value) {
    let ownerScope = variable.ownerView;
    let node = getViewNodeById(ownerScope.actorID);
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

  


  _getScopeByID: function (id) {
    let view = this._paramsView;
    for (let i = 0; i < view._store.length; i++) {
      let scope = view.getScopeAtIndex(i);
      if (scope.actorID === id)
        return scope;
    }
    return null;
  },

  


  _onMouseOver: function (e) {
    let id = WebAudioParamView._getScopeID(this);

    if (!id) return;

    WebAudioGraphView.focusNode(id);
  },

  


  _onMouseOut: function (e) {
    let id = WebAudioParamView._getScopeID(this);

    if (!id) return;

    WebAudioGraphView.blurNode(id);
  },

  



  _getScopeID: function ($el) {
    let match = $el.parentNode.id.match(/\(([^\)]*)\)/);
    return match ? match[1] : null;
  },

  



  addNode: Task.async(function* (_, id) {
    let viewNode = getViewNodeById(id);
    let type = viewNode.type;

    let audioParamsTitle = type + " (" + id + ")";
    let paramsView = this._paramsView;
    let paramsScopeView = paramsView.addScope(audioParamsTitle);

    paramsScopeView.actorID = id;
    paramsScopeView.expanded = false;

    paramsScopeView.addEventListener("mouseover", this._onMouseOver, false);
    paramsScopeView.addEventListener("mouseout", this._onMouseOut, false);

    let params = yield viewNode.getParams();
    params.forEach(({ param, value }) => {
      let descriptor = { value: value };
      paramsScopeView.addItem(param, descriptor);
    });

    window.emit(EVENTS.UI_ADD_NODE_LIST, id);
  }),

  



  removeNode: Task.async(function* (viewNode) {

  })
};







function findGraphNodeParent (el) {
  while (!el.classList.contains("nodes")) {
    if (el.classList.contains("audionode"))
      return el;
    else
      el = el.parentNode;
  }
  return null;
}
