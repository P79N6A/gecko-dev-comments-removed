


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
    this._onGraphClick = this._onGraphClick.bind(this);
    this._onThemeChange = this._onThemeChange.bind(this);
    this._onStartContext = this._onStartContext.bind(this);
    this._onEvent = this._onEvent.bind(this);

    this.draw = debounce(this.draw.bind(this), GRAPH_DEBOUNCE_TIMER);
    $("#graph-target").addEventListener("click", this._onGraphClick, false);

    window.on(EVENTS.THEME_CHANGE, this._onThemeChange);
    window.on(EVENTS.START_CONTEXT, this._onStartContext);
    gAudioNodes.on("*", this._onEvent);
  },

  


  destroy: function() {
    
    
    if (this._zoomBinding) {
      this._zoomBinding.on("zoom", null);
    }
    $("#graph-target").removeEventListener("click", this._onGraphClick, false);

    window.off(EVENTS.THEME_CHANGE, this._onThemeChange);
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

  



  _bypassNode: function (node, enabled) {
    let el = this._getNodeByID(node.id);
    el.classList[enabled ? "add" : "remove"]("bypassed");
  },

  




  draw: function () {
    
    this.clearGraph();

    let graph = new dagreD3.Digraph();
    let renderer = new dagreD3.Renderer();
    gAudioNodes.populateGraph(graph);

    
    let oldDrawNodes = renderer.drawNodes();
    renderer.drawNodes(function(graph, root) {
      let svgNodes = oldDrawNodes(graph, root);
      svgNodes.each(function (n) {
        let node = graph.node(n);
        let classString = "audionode type-" + node.type + (node.bypassed ? " bypassed" : "");
        this.setAttribute("class", classString);
        this.setAttribute("data-id", node.id);
        this.setAttribute("data-type", node.type);
      });
      return svgNodes;
    });

    
    let oldDrawEdgePaths = renderer.drawEdgePaths();
    let defaultClasses = "edgePath enter";

    renderer.drawEdgePaths(function(graph, root) {
      let svgEdges = oldDrawEdgePaths(graph, root);
      svgEdges.each(function (e) {
        let edge = graph.edge(e);

        
        
        let edgeClass = defaultClasses + (edge.param ? (" param-connection " + edge.param) : "");

        this.setAttribute("data-source", edge.source);
        this.setAttribute("data-target", edge.target);
        this.setAttribute("data-param", edge.param ? edge.param : null);
        this.setAttribute("class", edgeClass);
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
      if (DevToolsUtils.testing) {
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
    
    
    if (eventName === "bypass") {
      this._bypassNode.apply(this, args);
    }
    if (~GRAPH_REDRAW_EVENTS.indexOf(eventName)) {
      this.draw();
    }
  },

  


  _onThemeChange: function (eventName, theme) {
    let markerColor = MARKER_STYLING[theme];
    let marker = $("#arrowhead");
    if (marker) {
      marker.setAttribute("style", "fill: " + markerColor);
    }
  },

  





  _onGraphClick: function (e) {
    let node = findGraphNodeParent(e.target);
    
    
    if (!node)
      return;

    let id = node.getAttribute("data-id");

    this.focusNode(id);
    window.emit(EVENTS.UI_SELECT_NODE, id);
  }
};
