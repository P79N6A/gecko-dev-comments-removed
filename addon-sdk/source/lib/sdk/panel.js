


"use strict";



module.metadata = {
  "stability": "stable",
  "engines": {
    "Firefox": "*"
  }
};

const { Ci } = require("chrome");
const { setTimeout } = require('./timers');
const { isPrivateBrowsingSupported } = require('./self');
const { isWindowPBSupported } = require('./private-browsing/utils');
const { Class } = require("./core/heritage");
const { merge } = require("./util/object");
const { WorkerHost } = require("./content/utils");
const { Worker } = require("./content/worker");
const { Disposable } = require("./core/disposable");
const { WeakReference } = require('./core/reference');
const { contract: loaderContract } = require("./content/loader");
const { contract } = require("./util/contract");
const { on, off, emit, setListeners } = require("./event/core");
const { EventTarget } = require("./event/target");
const domPanel = require("./panel/utils");
const { events } = require("./panel/events");
const systemEvents = require("./system/events");
const { filter, pipe, stripListeners } = require("./event/utils");
const { getNodeView, getActiveView } = require("./view/core");
const { isNil, isObject, isNumber } = require("./lang/type");
const { getAttachEventType } = require("./content/utils");
const { number, boolean, object } = require('./deprecated/api-utils');
const { Style } = require("./stylesheet/style");
const { attach, detach } = require("./content/mod");

let isRect = ({top, right, bottom, left}) => [top, right, bottom, left].
  some(value => isNumber(value) && !isNaN(value));

let isSDKObj = obj => obj instanceof Class;

let rectContract = contract({
  top: number,
  right: number,
  bottom: number,
  left: number
});

let position = {
  is: object,
  map: v => (isNil(v) || isSDKObj(v) || !isObject(v)) ? v : rectContract(v),
  ok: v => isNil(v) || isSDKObj(v) || (isObject(v) && isRect(v)),
  msg: 'The option "position" must be a SDK object registered as anchor; ' +
        'or an object with one or more of the following keys set to numeric ' +
        'values: top, right, bottom, left.'
}

let displayContract = contract({
  width: number,
  height: number,
  focus: boolean,
  position: position
});

let panelContract = contract(merge({
  
  
  contentStyle: merge(Object.create(loaderContract.rules.contentScript), {
    msg: 'The `contentStyle` option must be a string or an array of strings.'
  }),
  contentStyleFile: merge(Object.create(loaderContract.rules.contentScriptFile), {
    msg: 'The `contentStyleFile` option must be a local URL or an array of URLs'
  }),
  contextMenu: boolean
}, displayContract.rules, loaderContract.rules));


function isDisposed(panel) !views.has(panel);

let panels = new WeakMap();
let models = new WeakMap();
let views = new WeakMap();
let workers = new WeakMap();
let styles = new WeakMap();

const viewFor = (panel) => views.get(panel);
const modelFor = (panel) => models.get(panel);
const panelFor = (view) => panels.get(view);
const workerFor = (panel) => workers.get(panel);
const styleFor = (panel) => styles.get(panel);



let setupAutoHide = new function() {
  let refs = new WeakMap();

  return function setupAutoHide(panel) {
    
    
    function listener({subject}) {
      
      
      let view = viewFor(panel);
      if (!view) systemEvents.off("popupshowing", listener);
      else if (subject !== view) panel.hide();
    }

    
    
    
    systemEvents.on("popupshowing", listener);
    
    
    
    refs.set(panel, listener);
  }
}

const Panel = Class({
  implements: [
    
    
    panelContract.properties(modelFor),
    EventTarget,
    Disposable,
    WeakReference
  ],
  extends: WorkerHost(workerFor),
  setup: function setup(options) {
    let model = merge({
      defaultWidth: 320,
      defaultHeight: 240,
      focus: true,
      position: Object.freeze({}),
      contextMenu: false
    }, panelContract(options));
    models.set(this, model);

    if (model.contentStyle || model.contentStyleFile) {
      styles.set(this, Style({
        uri: model.contentStyleFile,
        source: model.contentStyle
      }));
    }

    
    let view = domPanel.make();
    panels.set(view, this);
    views.set(this, view);

    
    domPanel.setURL(view, model.contentURL);
    
    
    domPanel.allowContextMenu(view, model.contextMenu);

    setupAutoHide(this);

    
    setListeners(this, options);
    let worker = new Worker(stripListeners(options));
    workers.set(this, worker);

    
    pipe(worker, this);
  },
  dispose: function dispose() {
    this.hide();
    off(this);

    workerFor(this).destroy();
    detach(styleFor(this));

    domPanel.dispose(viewFor(this));

    
    
    
    views.delete(this);
  },
  
  get width() modelFor(this).width,
  set width(value) this.resize(value, this.height),
  
  get height() modelFor(this).height,
  set height(value) this.resize(this.width, value),

  
  get focus() modelFor(this).focus,

  
  get position() modelFor(this).position,
  
  
  get contextMenu() modelFor(this).contextMenu,
  set contextMenu(allow) {
    let model = modelFor(this);
    model.contextMenu = panelContract({ contextMenu: allow }).contextMenu;
    domPanel.allowContextMenu(viewFor(this), model.contextMenu);
  },
    
  get contentURL() modelFor(this).contentURL,
  set contentURL(value) {
    let model = modelFor(this);
    model.contentURL = panelContract({ contentURL: value }).contentURL;
    domPanel.setURL(viewFor(this), model.contentURL);
    
    
    workerFor(this).detach();
  },

  
  get isShowing() !isDisposed(this) && domPanel.isOpen(viewFor(this)),

  
  show: function show(options={}, anchor) {
    if (options instanceof Ci.nsIDOMElement) {
      [anchor, options] = [options, null];
    }

    if (anchor instanceof Ci.nsIDOMElement) {
      console.warn(
        "Passing a DOM node to Panel.show() method is an unsupported " +
        "feature that will be soon replaced. " +
        "See: https://bugzilla.mozilla.org/show_bug.cgi?id=878877"
      );
    }

    let model = modelFor(this);
    let view = viewFor(this);
    let anchorView = getNodeView(anchor || options.position || model.position);

    options = merge({
      position: model.position,
      width: model.width,
      height: model.height,
      defaultWidth: model.defaultWidth,
      defaultHeight: model.defaultHeight,
      focus: model.focus,
      contextMenu: model.contextMenu
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


getActiveView.define(Panel, viewFor);


let panelEvents = filter(events, ({target}) => panelFor(target));


let shows = filter(panelEvents, ({type}) => type === "popupshown");


let hides = filter(panelEvents, ({type}) => type === "popuphidden");





let ready = filter(panelEvents, ({type, target}) =>
  getAttachEventType(modelFor(panelFor(target))) === type);



let start = filter(panelEvents, ({type}) => type === "document-element-inserted");


on(shows, "data", ({target}) => emit(panelFor(target), "show"));

on(hides, "data", ({target}) => emit(panelFor(target), "hide"));

on(ready, "data", ({target}) => {
  let panel = panelFor(target);
  let window = domPanel.getContentDocument(target).defaultView;
  
  workerFor(panel).attach(window);
});

on(start, "data", ({target}) => {
  let panel = panelFor(target);
  let window = domPanel.getContentDocument(target).defaultView;
  
  attach(styleFor(panel), window);
});
