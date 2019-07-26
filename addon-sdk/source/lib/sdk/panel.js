


"use strict";



module.metadata = {
  "stability": "stable",
  "engines": {
    "Firefox": "*"
  }
};

const { Ci } = require("chrome");
const { validateOptions: valid } = require('./deprecated/api-utils');
const { setTimeout } = require('./timers');
const { isPrivateBrowsingSupported } = require('./self');
const { isWindowPBSupported } = require('./private-browsing/utils');
const { Class } = require("./core/heritage");
const { merge } = require("./util/object");
const { WorkerHost, Worker, detach, attach } = require("./worker/utils");
const { Disposable } = require("./core/disposable");
const { contract: loaderContract } = require("./content/loader");
const { contract } = require("./util/contract");
const { on, off, emit, setListeners } = require("./event/core");
const { EventTarget } = require("./event/target");
const domPanel = require("./panel/utils");
const { events } = require("./panel/events");
const systemEvents = require("./system/events");
const { filter, pipe } = require("./event/utils");
const { getNodeView, getActiveView } = require("./view/core");
const { isNil, isObject } = require("./lang/type");

if (isPrivateBrowsingSupported && isWindowPBSupported)
  throw Error('The panel module cannot be used with per-window private browsing at the moment, see Bug 816257');

let isArray = Array.isArray;
let assetsURI = require("./self").data.url();

function isAddonContent({ contentURL }) {
  return typeof(contentURL) === "string" && contentURL.indexOf(assetsURI) === 0;
}

function hasContentScript({ contentScript, contentScriptFile }) {
  return (isArray(contentScript) ? contentScript.length > 0 :
         !!contentScript) ||
         (isArray(contentScriptFile) ? contentScriptFile.length > 0 :
         !!contentScriptFile);
}

function requiresAddonGlobal(model) {
  return isAddonContent(model) && !hasContentScript(model);
}

function getAttachEventType(model) {
  let when = model.contentScriptWhen;
  return requiresAddonGlobal(model) ? "sdk-panel-content-changed" :
         when === "start" ? "sdk-panel-content-changed" :
         when === "end" ? "sdk-panel-document-loaded" :
         when === "ready" ? "sdk-panel-content-loaded" :
         null;
}


let number = { is: ['number', 'undefined', 'null'] };
let boolean = { is: ['boolean', 'undefined', 'null'] };

let rectContract = contract({
  top: number,
  right: number,
  bottom: number,
  left: number
});

let rect = {
  is: ['object', 'undefined', 'null'],
  map: function(v) isNil(v) || !isObject(v) ? v : rectContract(v)
}

let displayContract = contract({
  width: number,
  height: number,
  focus: boolean,
  position: rect
});

let panelContract = contract(merge({}, displayContract.rules, loaderContract.rules));


function isDisposed(panel) !views.has(panel);

let panels = new WeakMap();
let models = new WeakMap();
let views = new WeakMap();
let workers = new WeakMap();

function viewFor(panel) views.get(panel)
function modelFor(panel) models.get(panel)
function panelFor(view) panels.get(view)
function workerFor(panel) workers.get(panel)

getActiveView.define(Panel, viewFor);



let setupAutoHide = new function() {
  let refs = new WeakMap();

  return function setupAutoHide(panel) {
    
    
    function listener({subject}) {
      
      
      let view = viewFor(panel);
      if (!view) systemEvents.off("sdk-panel-show", listener);
      else if (subject !== view) panel.hide();
    }

    
    
    
    systemEvents.on("sdk-panel-show", listener);
    
    
    
    refs.set(panel, listener);
  }
}

const Panel = Class({
  implements: [
    
    
    panelContract.properties(modelFor),
    EventTarget,
    Disposable
  ],
  extends: WorkerHost(workerFor),
  setup: function setup(options) {
    let model = merge({
      defaultWidth: 320,
      defaultHeight: 240,
      focus: true,
      position: Object.freeze({}),
    }, panelContract(options));
    models.set(this, model);

    
    setListeners(this, options);

    
    let view = domPanel.make();
    panels.set(view, this);
    views.set(this, view);

    
    domPanel.setURL(view, model.contentURL);

    setupAutoHide(this);

    let worker = new Worker(options);
    workers.set(this, worker);

    
    pipe(worker, this);
  },
  dispose: function dispose() {
    this.hide();
    off(this);

    detach(workerFor(this));

    domPanel.dispose(viewFor(this));

    
    
    
    views.delete(this);
  },
  
  get width() modelFor(this).width,
  set width(value) this.resize(value, this.height),
  
  get height() modelFor(this).height,
  set height(value) this.resize(this.width, value),

  
  get focus() modelFor(this).focus,

  
  get position() modelFor(this).position,

  get contentURL() modelFor(this).contentURL,
  set contentURL(value) {
    let model = modelFor(this);
    model.contentURL = panelContract({ contentURL: value }).contentURL;
    domPanel.setURL(viewFor(this), model.contentURL);
  },

  
  get isShowing() !isDisposed(this) && domPanel.isOpen(viewFor(this)),

  
  show: function show(options, anchor) {
    let model = modelFor(this);
    let view = viewFor(this);
    let anchorView = getNodeView(anchor);

    options = merge({
      position: model.position,
      width: model.width,
      height: model.height,
      defaultWidth: model.defaultWidth,
      defaultHeight: model.defaultHeight,
      focus: model.focus
    }, displayContract(options));

    if (!isDisposed(this))
      domPanel.show(view, options, anchorView);

    return this;
  },

  
  hide: function hide() {
    
    domPanel.close(viewFor(this));

    return this;
  },

  
  resize: function resize(width, height) {
    let model = modelFor(this);
    let view = viewFor(this);
    let change = panelContract({
      width: width || model.width || model.defaultWidth,
      height: height || model.height || model.defaultHeight
    });

    model.width = change.width
    model.height = change.height

    domPanel.resize(view, model.width, model.height);

    return this;
  }
});
exports.Panel = Panel;


let panelEvents = filter(function({target}) panelFor(target), events);


let shows = filter(function({type}) type === "sdk-panel-shown", panelEvents);


let hides = filter(function({type}) type === "sdk-panel-hidden", panelEvents);





let ready = filter(function({type, target})
  getAttachEventType(modelFor(panelFor(target))) === type, panelEvents);


let change = filter(function({type}) type === "sdk-panel-content-changed",
                    panelEvents);


on(shows, "data", function({target}) emit(panelFor(target), "show"));
on(hides, "data", function({target}) emit(panelFor(target), "hide"));

on(ready, "data", function({target}) {
  let worker = workerFor(panelFor(target));
  attach(worker, domPanel.getContentDocument(target).defaultView);
});
