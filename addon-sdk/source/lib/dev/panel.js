



"use strict";

module.metadata = {
  "stability": "experimental"
};

const { Cu } = require("chrome");
const { Class } = require("../sdk/core/heritage");
const { curry } = require("../sdk/lang/functional");
const { EventTarget } = require("../sdk/event/target");
const { Disposable, setup, dispose } = require("../sdk/core/disposable");
const { emit, off, setListeners } = require("../sdk/event/core");
const { when } = require("../sdk/event/utils");
const { getFrameElement } = require("../sdk/window/utils");
const { contract, validate } = require("../sdk/util/contract");
const { data: { url: resolve }} = require("../sdk/self");
const { identify } = require("../sdk/ui/id");
const { isLocalURL, URL } = require("../sdk/url");
const { encode } = require("../sdk/base64");
const { marshal, demarshal } = require("./ports");
const { fromTarget } = require("./debuggee");
const { removed } = require("../sdk/dom/events");
const { id: addonID } = require("../sdk/self");
const { viewFor } = require("../sdk/view/core");
const { createView } = require("./panel/view");

const OUTER_FRAME_URI = module.uri.replace(/\.js$/, ".html");
const FRAME_SCRIPT = module.uri.replace("/panel.js", "/frame-script.js");
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const HTML_NS = "http://www.w3.org/1999/xhtml";

const makeID = name =>
  ("dev-panel-" + addonID + "-" + name).
  split("/").join("-").
  split(".").join("-").
  split(" ").join("-").
  replace(/[^A-Za-z0-9_\-]/g, "");




const managers = new WeakMap();

const managerFor = x => managers.get(x);



const panels = new WeakMap();
const panelFor = frame => panels.get(frame);


const debuggees = new WeakMap();
const debuggeeFor = panel => debuggees.get(panel);

const frames = new WeakMap();
const frameFor = panel => frames.get(panel);

const setAttributes = (node, attributes) => {
  for (var key in attributes)
    node.setAttribute(key, attributes[key]);
};

const onStateChange = ({target, data}) => {
  const panel = panelFor(target);
  panel.readyState = data.readyState;
  emit(panel, data.type, { target: panel, type: data.type });
};




const onPortMessage = ({data, target}) => {
  const port = demarshal(target, data.port);
  if (port)
    port.postMessage(data.message);
};



const onFrameRemove = frame => {
  panelFor(frame).destroy();
};

const onFrameInited = frame => {
  frame.style.visibility = "visible";
}

const inited = frame => new Promise(resolve => {
  const { messageManager } = frame.frameLoader;
  const listener = message => {
    messageManager.removeMessageListener("sdk/event/ready", listener);
    resolve(frame);
  };
  messageManager.addMessageListener("sdk/event/ready", listener);
});

const getTarget = ({target}) => target;

const Panel = Class({
  extends: Disposable,
  implements: [EventTarget],
  get id() {
    return makeID(this.name || this.label);
  },
  readyState: "uninitialized",
  ready: function() {
    const { readyState } = this;
    const isReady = readyState === "complete" ||
                    readyState === "interactive";
    return isReady ? Promise.resolve(this) :
           when(this, "ready").then(getTarget);
  },
  loaded: function() {
    const { readyState } = this;
    const isLoaded = readyState === "complete";
    return isLoaded ? Promise.resolve(this) :
           when(this, "load").then(getTarget);
  },
  unloaded: function() {
    const { readyState } = this;
    const isUninitialized = readyState === "uninitialized";
    return isUninitialized ? Promise.resolve(this) :
           when(this, "unload").then(getTarget);
  },
  postMessage: function(data, ports) {
    const manager = managerFor(this);
    manager.sendAsyncMessage("sdk/event/message", {
      type: "message",
      bubbles: false,
      cancelable: false,
      data: data,
      origin: this.url,
      ports: ports.map(marshal(manager))
    });
  }
});
exports.Panel = Panel;

validate.define(Panel, contract({
  label: {
    is: ["string"],
    msg: "The `option.label` must be a provided"
  },
  tooltip: {
    is: ["string", "undefined"],
    msg: "The `option.tooltip` must be a string"
  },
  icon: {
    is: ["string"],
    map: x => x && resolve(x),
    ok: x => isLocalURL(x),
    msg: "The `options.icon` must be a valid local URI."
  },
  url: {
    map: x => resolve(x.toString()),
    is: ["string"],
    ok: x => isLocalURL(x),
    msg: "The `options.url` must be a valid local URI."
  }
}));

setup.define(Panel, (panel, {window, toolbox, url}) => {
  
  
  
  const original = getFrameElement(window);
  const container = original.parentNode;
  original.remove();
  const frame = createView(panel, container.ownerDocument);

  
  
  
  setAttributes(frame, {
    "id": original.id,
    "src": url,
    "flex": 1,
    "forceOwnRefreshDriver": "",
    "tooltip": "aHTMLTooltip"
  });
  frame.style.visibility = "hidden";
  frame.classList.add("toolbox-panel-iframe");
  
  
  if (!frame.parentNode)
    container.appendChild(frame);

  
  frames.set(panel, frame);

  
  panels.set(frame, panel);

  const debuggee = fromTarget(toolbox.target);
  
  debuggees.set(panel, debuggee);


  
  const { messageManager } = frame.frameLoader;
  messageManager.addMessageListener("sdk/event/ready", onStateChange);
  messageManager.addMessageListener("sdk/event/load", onStateChange);
  messageManager.addMessageListener("sdk/event/unload", onStateChange);
  messageManager.addMessageListener("sdk/port/message", onPortMessage);
  messageManager.loadFrameScript(FRAME_SCRIPT, false);

  managers.set(panel, messageManager);

  
  removed(frame).then(onFrameRemove);
  
  inited(frame).then(onFrameInited);


  
  setListeners(panel, Object.getPrototypeOf(panel));


  panel.setup({ debuggee: debuggee });
});

createView.define(Panel, (panel, document) => {
  const frame = document.createElement("iframe");
  setAttributes(frame, {
    "sandbox": "allow-scripts",
    
    
    
    
    "type": "content",
    "transparent": true,
    "seamless": "seamless",
  });
  return frame;
});

dispose.define(Panel, function(panel) {
  debuggeeFor(panel).close();

  debuggees.delete(panel);
  managers.delete(panel);
  frames.delete(panel);
  panel.readyState = "destroyed";
  panel.dispose();
});

viewFor.define(Panel, frameFor);
