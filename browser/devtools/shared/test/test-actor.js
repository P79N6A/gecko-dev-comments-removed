



"use strict";



let { Cc, Ci, Cu, Cr } = require("chrome");
let {LayoutHelpers} = Cu.import("resource://gre/modules/devtools/LayoutHelpers.jsm", {});
let DOMUtils = Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
            .getService(Ci.mozIJSSubScriptLoader);
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {})
const {Task} = Cu.import("resource://gre/modules/Task.jsm", {});
let EventUtils = {};
loader.loadSubScript("chrome://marionette/content/EventUtils.js", EventUtils);

const protocol = require("devtools/server/protocol");
const {Arg, Option, method, RetVal, types} = protocol;

let dumpn = msg => {
  dump(msg + "\n");
}









function getHighlighterCanvasFrameHelper(conn, actorID) {
  let actor = conn.getActor(actorID);
  if (actor && actor._highlighter) {
    return actor._highlighter.markup;
  }
}

const TestActor = exports.TestActor = protocol.ActorClass({
  typeName: "testActor",

  initialize: function(conn, tabActor, options) {
    this.conn = conn;
    this.tabActor = tabActor;
  },

  get content() {
    return this.tabActor.window;
  },

  






  _querySelector: function (selector) {
    let document = this.content.document;
    if (Array.isArray(selector)) {
      let fullSelector = selector.join(" >> ");
      while(selector.length > 1) {
        let str = selector.shift();
        let iframe = document.querySelector(str);
        if (!iframe) {
          throw new Error("Unable to find element with selector \"" + str + "\"" +
                          " (full selector:" + fullSelector + ")");
        }
        if (!iframe.contentWindow) {
          throw new Error("Iframe selector doesn't target an iframe \"" + str + "\"" +
                          " (full selector:" + fullSelector + ")");
        }
        document = iframe.contentWindow.document;
      }
      selector = selector.shift();
    }
    let node = document.querySelector(selector);
    if (!node) {
      throw new Error("Unable to find element with selector \"" + selector + "\"");
    }
    return node;
  },

  








  getHighlighterAttribute: protocol.method(function (nodeID, name, actorID) {
    let helper = getHighlighterCanvasFrameHelper(this.conn, actorID);
    if (helper) {
      return helper.getAttributeForElement(nodeID, name);
    }
  }, {
    request: {
      nodeID: Arg(0, "string"),
      name: Arg(1, "string"),
      actorID: Arg(2, "string")
    },
    response: {
      value: RetVal("string")
    }
  }),

  






  getHighlighterNodeTextContent: protocol.method(function (nodeID, actorID) {
    let value;
    let helper = getHighlighterCanvasFrameHelper(this.conn, actorID);
    if (helper) {
      value = helper.getTextContentForElement(nodeID);
    }
    return value;
  }, {
    request: {
      nodeID: Arg(0, "string"),
      actorID: Arg(1, "string")
    },
    response: {
      value: RetVal("string")
    }
  }),

  





  getSelectorHighlighterBoxNb: protocol.method(function (actorID) {
    let highlighter = this.conn.getActor(actorID);
    let {_highlighter: h} = highlighter;
    if (!h || !h._highlighters) {
      return null;
    } else {
      return h._highlighters.length;
    }
  }, {
    request: {
      highlighter: Arg(0, "string"),
    },
    response: {
      value: RetVal("number")
    }
  }),

  







  changeHighlightedNodeWaitForUpdate: protocol.method(function (name, value, actorID) {
    let deferred = promise.defer();

    let highlighter = this.conn.getActor(actorID);
    let {_highlighter: h} = highlighter;

    h.once("updated", () => {
      deferred.resolve();
    });

    h.currentNode.setAttribute(name, value);

    return deferred.promise;
  }, {
    request: {
      name: Arg(0, "string"),
      value: Arg(1, "string"),
      actorID: Arg(2, "string")
    },
    response: {}
  }),

  




  waitForHighlighterEvent: protocol.method(function (event, actorID) {
    let highlighter = this.conn.getActor(actorID);
    let {_highlighter: h} = highlighter;

    return h.once(event);
  }, {
    request: {
      event: Arg(0, "string"),
      actorID: Arg(1, "string")
    },
    response: {}
  }),

  






  changeZoomLevel: protocol.method(function (level, actorID) {
    dumpn("Zooming page to " + level);
    let deferred = promise.defer();

    if (actorID) {
      let actor = this.conn.getActor(actorID);
      let {_highlighter: h} = actor;
      h.once("updated", () => {
        deferred.resolve();
      });
    } else {
      deferred.resolve();
    }

    let docShell = this.content.QueryInterface(Ci.nsIInterfaceRequestor)
                               .getInterface(Ci.nsIWebNavigation)
                               .QueryInterface(Ci.nsIDocShell);
    docShell.contentViewer.fullZoom = level;

    return deferred.promise;
  }, {
    request: {
      level: Arg(0, "string"),
      actorID: Arg(1, "string"),
    },
    response: {}
  }),

  assertElementAtPoint: protocol.method(function (x, y, selector) {
    let helper = new LayoutHelpers(this.content);
    let elementAtPoint = helper.getElementFromPoint(this.content.document, x, y);
    if (!elementAtPoint) {
      throw new Error("Unable to find element at (" + x + ", " + y + ")");
    }
    let node = this._querySelector(selector);
    return node == elementAtPoint;
  }, {
    request: {
      x: Arg(0, "number"),
      y: Arg(1, "number"),
      selector: Arg(2, "string")
    },
    response: {
      value: RetVal("boolean")
    }
  }),


  





  getAllAdjustedQuads: protocol.method(function(selector) {
    let regions = {};
    let helper = new LayoutHelpers(this.content);
    let node = this._querySelector(selector);
    for (let boxType of ["content", "padding", "border", "margin"]) {
      regions[boxType] = helper.getAdjustedQuads(node, boxType);
    }

    return regions;
  }, {
    request: {
      selector: Arg(0, "string")
    },
    response: {
      value: RetVal("json")
    }
  }),

  










  synthesizeMouse: protocol.method(function({ selector, x, y, center, options }) {
    let node = this._querySelector(selector);

    if (center) {
      EventUtils.synthesizeMouseAtCenter(node, options, node.ownerDocument.defaultView);
    } else {
      EventUtils.synthesizeMouse(node, x, y, options, node.ownerDocument.defaultView);
    }
  }, {
    request: {
      object: Arg(0, "json")
    },
    response: {}
  }),

  




  synthesizeKey: protocol.method(function ({key, options, content}) {
    EventUtils.synthesizeKey(key, options, this.content);
  }, {
    request: {
      args: Arg(0, "json")
    },
    response: {}
  }),

  





  hasPseudoClassLock: protocol.method(function (selector, pseudo) {
    let node = this._querySelector(selector);
    return DOMUtils.hasPseudoClassLock(node, pseudo);
  }, {
    request: {
      selector: Arg(0, "string"),
      pseudo: Arg(1, "string")
    },
    response: {
      value: RetVal("boolean")
    }
  }),

  loadAndWaitForCustomEvent: protocol.method(function (url) {
    let deferred = promise.defer();
    let self = this;
    
    
    this.tabActor.chromeEventHandler.addEventListener("DOMWindowCreated", function onWindowCreated() {
      self.tabActor.chromeEventHandler.removeEventListener("DOMWindowCreated", onWindowCreated);
      self.content.addEventListener("test-page-processing-done", function onEvent() {
        self.content.removeEventListener("test-page-processing-done", onEvent);
        deferred.resolve();
      });
    });

    this.content.location = url;
    return deferred.promise;
  }, {
    request: {
      url: Arg(0, "string")
    },
    response: {}
  }),

  hasNode: protocol.method(function (selector) {
    try {
      
      this._querySelector(selector);
      return true;
    } catch(e) {
      return false;
    }
  }, {
    request: {
      selector: Arg(0, "string")
    },
    response: {
      value: RetVal("boolean")
    }
  }),

  




  getBoundingClientRect: protocol.method(function (selector) {
    let node = this._querySelector(selector);
    return node.getBoundingClientRect();
  }, {
    request: {
      selector: Arg(0, "string"),
    },
    response: {
      value: RetVal("json")
    }
  }),

  





  setProperty: protocol.method(function (selector, property, value) {
    let node = this._querySelector(selector);
    node[property] = value;
  }, {
    request: {
      selector: Arg(0, "string"),
      property: Arg(1, "string"),
      value: Arg(2, "string")
    },
    response: {}
  }),

  





  getProperty: protocol.method(function (selector, property) {
    let node = this._querySelector(selector);
    return node[property];
  }, {
    request: {
      selector: Arg(0, "string"),
      property: Arg(1, "string")
    },
    response: {
      value: RetVal("string")
    }
  }),

  



  reloadFrame: protocol.method(function (selector) {
    let node = this._querySelector(selector);

    let deferred = promise.defer();

    let onLoad = function () {
      node.removeEventListener("load", onLoad);
      deferred.resolve();
    };
    node.addEventListener("load", onLoad);

    node.contentWindow.location.reload();
    return deferred.promise;
  }, {
    request: {
      selector: Arg(0, "string"),
    },
    response: {}
  }),

  




  eval: protocol.method(function (js) {
    
    let sb = Cu.Sandbox(this.content, { sandboxPrototype: this.content });
    return Cu.evalInSandbox(js, sb);
  }, {
    request: {
      js: Arg(0, "string")
    },
    response: {
      value: RetVal("nullable:json")
    }
  }),

  










  scrollWindow: protocol.method(function (x, y, relative) {
    if (isNaN(x) || isNaN(y)) {
      return {};
    }

    let deferred = promise.defer();
    this.content.addEventListener("scroll", function onScroll(event) {
      this.removeEventListener("scroll", onScroll);

      let data = {x: this.content.scrollX, y: this.content.scrollY};
      deferred.resolve(data);
    });

    this.content[relative ? "scrollBy" : "scrollTo"](x, y);

    return deferred.promise;
  }, {
    request: {
      x: Arg(0, "number"),
      y: Arg(1, "number"),
      relative: Arg(2, "nullable:boolean"),
    },
    response: {
      value: RetVal("json")
    }
  }),
});

const TestActorFront = exports.TestActorFront = protocol.FrontClass(TestActor, {
  initialize: function(client, { testActor }, toolbox) {
    protocol.Front.prototype.initialize.call(this, client, { actor: testActor });
    this.manage(this);
    this.toolbox = toolbox;
  },

  





  zoomPageTo: function(level) {
    return this.changeZoomLevel(level, this.toolbox.highlighter.actorID);
  },

  changeHighlightedNodeWaitForUpdate: protocol.custom(function(name, value, highlighter) {
    return this._changeHighlightedNodeWaitForUpdate(name, value, (highlighter || this.toolbox.highlighter).actorID);
  }, {
    impl: "_changeHighlightedNodeWaitForUpdate"
  }),

  






  getHighlighterNodeAttribute: function(nodeID, name, highlighter) {
    return this.getHighlighterAttribute(nodeID, name, (highlighter || this.toolbox.highlighter).actorID);
  },

  getHighlighterNodeTextContent: protocol.custom(function(nodeID, highlighter) {
    return this._getHighlighterNodeTextContent(nodeID, (highlighter || this.toolbox.highlighter).actorID);
  }, {
    impl: "_getHighlighterNodeTextContent"
  }),

  


  isHighlighting: function() {
    return this.getHighlighterNodeAttribute("box-model-elements", "hidden")
      .then(value => value === null);
  },

  assertHighlightedNode: Task.async(function* (selector) {
    let {visible, content} = yield this._getBoxModelStatus();
    let points = content.points;
    if (visible) {
      let x = (points.p1.x + points.p2.x + points.p3.x + points.p4.x) / 4;
      let y = (points.p1.y + points.p2.y + points.p3.y + points.p4.y) / 4;

      return this.assertElementAtPoint(x, y, selector);
    } else {
      return false;
    }
  }),

  







  isNodeCorrectlyHighlighted: Task.async(function*(selector, is, prefix="") {
    prefix += (prefix ? " " : "") + selector + " ";

    let boxModel = yield this._getBoxModelStatus();
    let regions = yield this.getAllAdjustedQuads(selector);

    for (let boxType of ["content", "padding", "border", "margin"]) {
      let [quad] = regions[boxType];
      for (let point in boxModel[boxType].points) {
        is(boxModel[boxType].points[point].x, quad[point].x,
          prefix + boxType + " point " + point + " x coordinate is correct");
        is(boxModel[boxType].points[point].y, quad[point].y,
          prefix + boxType + " point " + point + " y coordinate is correct");
      }
    }
  }),

  


  getSimpleBorderRect: Task.async(function*(toolbox) {
    let {border} = yield this._getBoxModelStatus(toolbox);
    let {p1, p2, p3, p4} = border.points;

    return {
      top: p1.y,
      left: p1.x,
      width: p2.x - p1.x,
      height: p4.y - p1.y
    };
  }),

  



  _getBoxModelStatus: Task.async(function*() {
    let isVisible = yield this.isHighlighting();

    let ret = {
      visible: isVisible
    };

    for (let region of ["margin", "border", "padding", "content"]) {
      let points = yield this._getPointsForRegion(region);
      let visible = yield this._isRegionHidden(region);
      ret[region] = {points, visible};
    }

    ret.guides = {};
    for (let guide of ["top", "right", "bottom", "left"]) {
      ret.guides[guide] = yield this._getGuideStatus(guide);
    }

    return ret;
  }),

  



  _getPointsForRegion: Task.async(function*(region) {
    let d = yield this.getHighlighterNodeAttribute("box-model-" + region, "d");

    let polygons = d.match(/M[^M]+/g);
    if (!polygons) {
      return null;
    }

    let points = polygons[0].trim().split(" ").map(i => {
      return i.replace(/M|L/, "").split(",")
    });

    return {
      p1: {
        x: parseFloat(points[0][0]),
        y: parseFloat(points[0][1])
      },
      p2: {
        x: parseFloat(points[1][0]),
        y: parseFloat(points[1][1])
      },
      p3: {
        x: parseFloat(points[2][0]),
        y: parseFloat(points[2][1])
      },
      p4: {
        x: parseFloat(points[3][0]),
        y: parseFloat(points[3][1])
      }
    };
  }),

  



  _isRegionHidden: Task.async(function*(region) {
    let value = yield this.getHighlighterNodeAttribute("box-model-" + region, "hidden");
    return value !== null;
  }),

  _getGuideStatus: Task.async(function*(location) {
    let id = "box-model-guide-" + location;

    let hidden = yield this.getHighlighterNodeAttribute(id, "hidden");
    let x1 = yield this.getHighlighterNodeAttribute(id, "x1");
    let y1 = yield this.getHighlighterNodeAttribute(id, "y1");
    let x2 = yield this.getHighlighterNodeAttribute(id, "x2");
    let y2 = yield this.getHighlighterNodeAttribute(id, "y2");

    return {
      visible: !hidden,
      x1: x1,
      y1: y1,
      x2: x2,
      y2: y2
    };
  }),

  





  getGuidesRectangle: Task.async(function*() {
    let tGuide = yield this._getGuideStatus("top");
    let rGuide = yield this._getGuideStatus("right");
    let bGuide = yield this._getGuideStatus("bottom");
    let lGuide = yield this._getGuideStatus("left");

    if (!tGuide.visible || !rGuide.visible || !bGuide.visible || !lGuide.visible) {
      return null;
    }

    return {
      p1: {x: lGuide.x1, y: tGuide.y1},
      p2: {x: rGuide.x1, y: tGuide. y1},
      p3: {x: rGuide.x1, y: bGuide.y1},
      p4: {x: lGuide.x1, y: bGuide.y1}
    };
  }),

  waitForHighlighterEvent: protocol.custom(function(event) {
    return this._waitForHighlighterEvent(event, this.toolbox.highlighter.actorID);
  }, {
    impl: "_waitForHighlighterEvent"
  }),

  









  getHighlighterRegionPath: Task.async(function*(region, highlighter) {
    let d = yield this.getHighlighterNodeAttribute("box-model-" + region, "d", highlighter);
    if (!d) {
      return {d: null};
    }

    let polygons = d.match(/M[^M]+/g);
    if (!polygons) {
      return {d};
    }

    let points = [];
    for (let polygon of polygons) {
      points.push(polygon.trim().split(" ").map(i => {
        return i.replace(/M|L/, "").split(",")
      }));
    }

    return {d, points};
  })
});
