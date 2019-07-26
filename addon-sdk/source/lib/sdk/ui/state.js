


'use strict';



module.metadata = {
  'stability': 'experimental',
  'engines': {
    'Firefox': '*'
  }
};

const { Ci } = require('chrome');

const events = require('../event/utils');
const { events: browserEvents } = require('../browser/events');
const { events: tabEvents } = require('../tab/events');

const { windows, isInteractive } = require('../window/utils');
const { BrowserWindow, browserWindows } = require('../windows');
const { windowNS } = require('../window/namespace');
const { Tab } = require('../tabs/tab');
const { getActiveTab, getOwnerWindow, getTabs, getTabId } = require('../tabs/utils');

const { ignoreWindow } = require('../private-browsing/utils');

const { freeze } = Object;
const { merge } = require('../util/object');
const { on, off, emit } = require('../event/core');

const { add, remove, has, clear, iterator } = require("../lang/weak-set");
const { isNil, instanceOf } = require('../lang/type');

const components = new WeakMap();

const ERR_UNREGISTERED = 'The state cannot be set or get. ' +
  'The object may be not be registered, or may already have been unloaded.';




function getChromeWindow(sdkWindow) windowNS(sdkWindow).window;




function getChromeTab(sdkTab) {
  for (let tab of getTabs()) {
    if (sdkTab.id === getTabId(tab))
      return tab;
  }
  return null;
}

const isWindow = thing => thing instanceof Ci.nsIDOMWindow;
const isTab = thing => thing.tagName && thing.tagName.toLowerCase() === "tab";
const isActiveTab = thing => isTab(thing) && thing === getActiveTab(getOwnerWindow(thing));
const isWindowEnumerable = window => !ignoreWindow(window);

function getStateFor(component, target) {
  if (!isRegistered(component))
    throw new Error(ERR_UNREGISTERED);

  if (!components.has(component))
    return null;

  let states = components.get(component);

  let componentState = states.get(component);
  let windowState = null;
  let tabState = null;

  if (target) {
    
    if (isTab(target)) {
      windowState = states.get(getOwnerWindow(target), null);

      if (states.has(target)) {
        
        tabState = states.get(target);
      }
    }
    else if (isWindow(target) && states.has(target)) {
      
      windowState = states.get(target);
    }
  }

  return freeze(merge({}, componentState, windowState, tabState));
}
exports.getStateFor = getStateFor;

function setStateFor(component, target, state) {
  if (!isRegistered(component))
    throw new Error(ERR_UNREGISTERED);

  let targetWindows = [];
  let isComponentState = target === component;

  if (isWindow(target)) {
    targetWindows = [target];
  }
  else if (isActiveTab(target)) {
    targetWindows = [getOwnerWindow(target)];
  }
  else if (isComponentState) {
    targetWindows = windows('navigator:browser', { includePrivate: true}).filter(isInteractive);
  }
  else if (!isTab(target))
    throw new Error('target not allowed.');

  
  if (!components.has(component))
    components.set(component, new WeakMap());

  let states = components.get(component);

  if (state === null && !isComponentState) 
    states.delete(target);
  else {
    let base = isComponentState ? states.get(target) : null;
    states.set(target, freeze(merge({}, base, state)));
  }

  for (let window of targetWindows.filter(isWindowEnumerable)) {
    let tabState = getStateFor(component, getActiveTab(window));

    emit(component.constructor, 'render', component, window, tabState);
  }
}






exports.setStateFor = setStateFor;

function render(component, targetWindows) {
  if (!targetWindows)
    targetWindows = windows('navigator:browser', { includePrivate: true}).filter(isInteractive);
  else
    targetWindows = [].concat(targetWindows);

  for (let window of targetWindows.filter(isWindowEnumerable)) {
    let tabState = getStateFor(component, getActiveTab(window));

    emit(component.constructor, 'render', component, window, tabState);
  }
}
exports.render = render;

function properties(contract) {
  let { rules } = contract;
  let descriptor = Object.keys(rules).reduce(function(descriptor, name) {
    descriptor[name] = {
      get: function() { return getStateFor(this)[name] },
      set: function(value) {
        let changed = {};
        changed[name] = value;

        setStateFor(this, this, contract(changed));
      }
    }
    return descriptor;
  }, {});

  return Object.create(Object.prototype, descriptor);
}
exports.properties = properties;

function state(contract) {
  return {
    state: function state(target, state) {
      
      let isGet = arguments.length < 2;

      if (instanceOf(target, BrowserWindow))
        target = getChromeWindow(target);
      else if (instanceOf(target, Tab))
        target = getChromeTab(target);
      else if (target !== this && !isNil(target))
        throw new Error('target not allowed.');

      if (isGet)
        return getStateFor(this, target);

      
      setStateFor(this, target, contract(state));
    }
  }
}
exports.state = state;

function register(component, state) {
  add(components, component);
  setStateFor(component, component, state);
}
exports.register = register;

function unregister(component) remove(components, component);
exports.unregister = unregister;

function isRegistered(component) has(components, component);
exports.isRegistered = isRegistered;

let tabSelect = events.filter(tabEvents, function(e) e.type === 'TabSelect');
let tabClose = events.filter(tabEvents, function(e) e.type === 'TabClose');
let windowOpen = events.filter(browserEvents, function(e) e.type === 'load');
let windowClose = events.filter(browserEvents, function(e) e.type === 'close');

let close = events.merge([tabClose, windowClose]);

on(windowOpen, 'data', function({target: window}) {
  if (ignoreWindow(window)) return;

  let tab = getActiveTab(window);

  for (let component of iterator(components)) {
    emit(component.constructor, 'render', component, window, getStateFor(component, tab));
  }
});

on(tabSelect, 'data', function({target: tab}) {
  let window = getOwnerWindow(tab);

  if (ignoreWindow(window)) return;

  for (let component of iterator(components)) {
    emit(component.constructor, 'render', component, window, getStateFor(component, tab));
  }
});

on(close, 'data', function({target}) {
  for (let component of iterator(components)) {
    components.get(component).delete(target);
  }
});
