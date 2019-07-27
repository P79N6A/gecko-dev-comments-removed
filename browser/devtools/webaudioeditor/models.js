


"use strict";



const { emit: coreEmit } = require("sdk/event/core");









const AudioNodeModel = Class({
  extends: EventTarget,

  
  collection: null,

  initialize: function (actor) {
    this.actor = actor;
    this.id = actor.actorID;
    this.connections = [];
  },

  





  setup: Task.async(function* () {
    yield this.getType();
  }),

  





  getType: Task.async(function* () {
    this.type = yield this.actor.getType();
    return this.type;
  }),

  









  connect: function (destination, param) {
    let edge = findWhere(this.connections, { destination: destination.id, param: param });

    if (!edge) {
      this.connections.push({ source: this.id, destination: destination.id, param: param });
      coreEmit(this, "connect", this, destination, param);
    }
  },

  


  disconnect: function () {
    this.connections.length = 0;
    coreEmit(this, "disconnect", this);
  },

  





  getParams: function () {
    return this.actor.getParams();
  },

  





  addToGraph: function (graph) {
    graph.addNode(this.id, {
      type: this.type,
      label: this.type.replace(/Node$/, ""),
      id: this.id
    });
  },

  







  addEdgesToGraph: function (graph) {
    for (let edge of this.connections) {
      let options = {
        source: this.id,
        target: edge.destination
      };

      
      
      
      
      if (edge.param) {
        options.label = options.param = edge.param;
      }

      graph.addEdge(null, this.id, edge.destination, options);
    }
  }
});











const AudioNodesCollection = Class({
  extends: EventTarget,

  model: AudioNodeModel,

  initialize: function () {
    this.models = new Set();
    this._onModelEvent = this._onModelEvent.bind(this);
  },

  





  forEach: function (fn) {
    this.models.forEach(fn);
  },

  












  add: Task.async(function* (obj) {
    let node = new this.model(obj);
    node.collection = this;
    yield node.setup();

    this.models.add(node);

    node.on("*", this._onModelEvent);
    coreEmit(this, "add", node);
    return node;
  }),

  





  remove: function (node) {
    this.models.delete(node);
    coreEmit(this, "remove", node);
  },

  


  reset: function () {
    this.models.clear();
  },

  







  get: function (id) {
    return findWhere(this.models, { id: id });
  },

  




  get length() {
    return this.models.size;
  },

  







  getInfo: function () {
    let info = {
      nodes: this.length,
      edges: 0,
      paramEdges: 0
    };

    this.models.forEach(node => {
      let paramEdgeCount = node.connections.filter(edge => edge.param).length;
      info.edges += node.connections.length - paramEdgeCount;
      info.paramEdges += paramEdgeCount;
    });
    return info;
  },

  





  populateGraph: function (graph) {
    this.models.forEach(node => node.addToGraph(graph));
    this.models.forEach(node => node.addEdgesToGraph(graph));
  },

  




  _onModelEvent: function (eventName, node, ...args) {
    if (eventName === "remove") {
      
      
      
      this.remove(node);
    } else {
      
      coreEmit(this, eventName, [node].concat(args));
    }
  }
});
