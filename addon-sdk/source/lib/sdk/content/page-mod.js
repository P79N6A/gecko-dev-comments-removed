


"use strict";

module.metadata = {
  "stability": "stable"
};

const { getAttachEventType } = require('../content/utils');
const { Class } = require('../core/heritage');
const { Disposable } = require('../core/disposable');
const { WeakReference } = require('../core/reference');
const { WorkerChild } = require('./worker-child');
const { EventTarget } = require('../event/target');
const { on, emit, once, setListeners } = require('../event/core');
const { on: domOn, removeListener: domOff } = require('../dom/events');
const { isRegExp, isUndefined } = require('../lang/type');
const { merge } = require('../util/object');
const { isBrowser, getFrames } = require('../window/utils');
const { getTabs, getTabContentWindow, getTabForContentWindow,
        getURI: getTabURI } = require('../tabs/utils');
const { ignoreWindow } = require('../private-browsing/utils');
const { Style } = require("../stylesheet/style");
const { attach, detach } = require("../content/mod");
const { has, hasAny } = require("../util/array");
const { Rules } = require("../util/rules");
const { List, addListItem, removeListItem } = require('../util/list');
const { when } = require("../system/unload");
const { uuid } = require('../util/uuid');
const { frames, process } = require('../remote/child');

const pagemods = new Map();
const styles = new WeakMap();
let styleFor = (mod) => styles.get(mod);


let modMatchesURI = (mod, uri) => mod.include.matchesAny(uri) && !mod.exclude.matchesAny(uri);





const ChildPageMod = Class({
  implements: [
    EventTarget,
    Disposable,
  ],
  setup: function PageMod(model) {
    merge(this, model);

    
    
    setListeners(this, model);

    function deserializeRules(rules) {
      for (let rule of rules) {
        yield rule.type == "string" ? rule.value
                                    : new RegExp(rule.pattern, rule.flags);
      }
    }

    let include = [...deserializeRules(this.include)];
    this.include = Rules();
    this.include.add.apply(this.include, include);

    let exclude = [...deserializeRules(this.exclude)];
    this.exclude = Rules();
    this.exclude.add.apply(this.exclude, exclude);

    if (this.contentStyle || this.contentStyleFile) {
      styles.set(this, Style({
        uri: this.contentStyleFile,
        source: this.contentStyle
      }));
    }

    pagemods.set(this.id, this);
    this.seenDocuments = new WeakMap();

    
    
    if (has(this.attachTo, 'existing'))
      applyOnExistingDocuments(this);
  },

  dispose: function() {
    let style = styleFor(this);
    if (style)
      detach(style);

    for (let i in this.include)
      this.include.remove(this.include[i]);

    pagemods.delete(this.id);
  }
});

function onContentWindow({ target: document }) {
  
  if (pagemods.size === 0)
    return;

  let window = document.defaultView;
  
  if (!window)
    return;

  
  let frame = this;
  
  if (!frame.isTab)
    return;

  
  
  if (ignoreWindow(window))
    return;

  for (let pagemod of pagemods.values()) {
    if (modMatchesURI(pagemod, window.location.href))
      onContent(pagemod, window);
  }
}
frames.addEventListener("DOMDocElementInserted", onContentWindow, true);

function applyOnExistingDocuments (mod) {
  for (let frame of frames) {
    
    let window = frame.content;
    
    
    if (!window || !window.frames)
      return;
    let uri = window.location.href;
    if (has(mod.attachTo, "top") && modMatchesURI(mod, uri))
      onContent(mod, window);
    if (has(mod.attachTo, "frame"))
      getFrames(window).
        filter(iframe => modMatchesURI(mod, iframe.location.href)).
        forEach(frame => onContent(mod, frame));
  }
}

function createWorker(mod, window) {
  let workerId = String(uuid());

  
  
  let frame = frames.getFrameForWindow(window.top);
  frame.port.emit('sdk/page-mod/worker-create', mod.id, {
    id: workerId,
    url: window.location.href
  });

  
  let worker = WorkerChild({
    id: workerId,
    window: window,
    contentScript: mod.contentScript,
    contentScriptFile: mod.contentScriptFile,
    contentScriptOptions: mod.contentScriptOptions
  });

  once(worker, 'detach', () => worker.destroy());
}

function onContent (mod, window) {
  let isTopDocument = window.top === window;
  
  if (isTopDocument && !has(mod.attachTo, "top"))
    return;
  
  if (!isTopDocument && !has(mod.attachTo, "frame"))
    return;

  
  let seen = mod.seenDocuments;
  if (seen.has(window.document))
    return;
  seen.set(window.document, true);

  let style = styleFor(mod);
  if (style)
    attach(style, window);

  
  
  if (isMatchingAttachState(mod, window)) {
    createWorker(mod, window);
    return;
  }

  let eventName = getAttachEventType(mod) || 'load';
  domOn(window, eventName, function onReady (e) {
    if (e.target.defaultView !== window)
      return;
    domOff(window, eventName, onReady, true);
    createWorker(mod, window);

    
    
    if (window.document.readyState == "complete") {
      mod.on('attach', worker => {
        try {
          worker.send('pageshow');
          emit(worker, 'pageshow');
        }
        catch (e) {
          
        }
      });
    }
  }, true);
}

function isMatchingAttachState (mod, window) {
  let state = window.document.readyState;
  return 'start' === mod.contentScriptWhen ||
      
      'complete' === state ||
      
      ('ready' === mod.contentScriptWhen && state === 'interactive')
}

process.port.on('sdk/page-mod/create', (process, model) => {
  if (pagemods.has(model.id))
    return;

  new ChildPageMod(model);
});

process.port.on('sdk/page-mod/destroy', (process, id) => {
  let mod = pagemods.get(id);
  if (mod)
    mod.destroy();
});
