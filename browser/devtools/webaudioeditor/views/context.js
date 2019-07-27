


"use strict";

const { debounce } = require("sdk/lang/functional");



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



const GRAPH_REDRAW_EVENTS = ["add", "connect", "disconnect", "remove"];




let ContextView = {
  


  initialize: function() {
    this._onGraphNodeClick = this._onGraphNodeClick.bind(this);
    this._onThemeChange = this._onThemeChange.bind(this);
    this._onNodeSelect = this._onNodeSelect.bind(this);
    this._onStartContext = this._onStartContext.bind(this);
    this._onEvent = this._onEvent.bind(this);

    this.draw = debounce(this.draw.bind(this), GRAPH_DEBOUNCE_TIMER);
    $('#graph-target').addEventListener('click', this._onGraphNodeClick, false);

    window.on(EVENTS.THEME_CHANGE, this._onThemeChange);
    window.on(EVENTS.UI_INSPECTOR_NODE_SET, this._onNodeSelect);
    window.on(EVENTS.START_CONTEXT, this._onStartContext);
    gAudioNodes.on("*", this._onEvent);
  },

  


  destroy: function() {
    
    
    if (this._zoomBinding) {
      this._zoomBinding.on("zoom", null);
    }
    $('#graph-target').removeEventListener('click', this._onGraphNodeClick, false);
    window.off(EVENTS.THEME_CHANGE, this._onThemeChange);
    window.off(EVENTS.UI_INSPECTOR_NODE_SET, this._onNodeSelect);
    window.off(EVENTS.START_CONTEXT, this._onStartContext);
    gAudioNodes.off("*", this._onEvent);
  },

  



  resetUI: function () {
    this.clearGraph();
    this.resetGraphTransform();
  },

  



  clearGraph: function () {
    $("#graph-target").innerHTML = "";
  },

  


  resetGraphTransform: function () {
    
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
    let renderer = new dagreD3.Renderer();
    gAudioNodes.populateGraph(graph);

    
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

      
      let currentNode = InspectorView.getCurrentAudioNode();
      if (currentNode) {
        this.focusNode(currentNode.id);
      }

      
      
      let info = {};
      if (gDevTools.testing) {
        info = gAudioNodes.getInfo();
      }
      window.emit(EVENTS.UI_GRAPH_RENDERED, info.nodes, info.edges, info.paramEdges);
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

      
      
      this.resetGraphTransform();
    }
  },

  



  



  _onStartContext: function () {
    this.draw();
  },

  



  _onEvent: function (eventName, ...args) {
    if (~GRAPH_REDRAW_EVENTS.indexOf(eventName)) {
      this.draw();
    }
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
