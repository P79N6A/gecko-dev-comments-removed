


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
const { events: stateEvents } = require('./state/events');

const { windows, isInteractive, getMostRecentBrowserWindow } = require('../window/utils');
const { getActiveTab, getOwnerWindow } = require('../tabs/utils');

const { ignoreWindow } = require('../private-browsing/utils');

const { freeze } = Object;
const { merge } = require('../util/object');
const { on, off, emit } = require('../event/core');

const { add, remove, has, clear, iterator } = require('../lang/weak-set');
const { isNil } = require('../lang/type');

const { viewFor } = require('../view/core');

const components = new WeakMap();

const ERR_UNREGISTERED = 'The state cannot be set or get. ' +
  'The object may be not be registered, or may already have been unloaded.';

const isWindow = thing => thing instanceof Ci.nsIDOMWindow;
const isTab = thing => thing.tagName && thing.tagName.toLowerCase() === 'tab';
const isActiveTab = thing => isTab(thing) && thing === getActiveTab(getOwnerWindow(thing));
const isEnumerable = window => !ignoreWindow(window);
const browsers = _ =>
  windows('navigator:browser', { includePrivate: true }).filter(isInteractive);
const getMostRecentTab = _ => getActiveTab(getMostRecentBrowserWindow());

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

  let isComponentState = target === component;
  let targetWindows = isWindow(target) ? [target] :
                      isActiveTab(target) ? [getOwnerWindow(target)] :
                      isComponentState ? browsers() :
                      isTab(target) ? [] :
                      null;

  if (!targetWindows)
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

  render(component, targetWindows);
}

function render(component, targetWindows) {
  targetWindows = targetWindows ? [].concat(targetWindows) : browsers();

  for (let window of targetWindows.filter(isEnumerable)) {
    let tabState = getStateFor(component, getActiveTab(window));

    emit(stateEvents, 'data', {
      type: 'render',
      target: component,
      window: window,
      state: tabState
    });

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
      let nativeTarget = target === 'window' ? getMostRecentBrowserWindow()
                          : target === 'tab' ? getMostRecentTab()
                          : viewFor(target);

      if (!nativeTarget && target !== this && !isNil(target))
        throw new Error('target not allowed.');

      target = nativeTarget || target;

      
      return arguments.length < 2
        ? getStateFor(this, target)
        : setStateFor(this, target, contract(state))
    }
  }
}
exports.state = state;

const register = (component, state) => {
  add(components, component);
  setStateFor(component, component, state);
}
exports.register = register;

const unregister = component => {
  remove(components, component);
}
exports.unregister = unregister;

const isRegistered = component => has(components, component);
exports.isRegistered = isRegistered;

let tabSelect = events.filter(tabEvents, e => e.type === 'TabSelect');
let tabClose = events.filter(tabEvents, e => e.type === 'TabClose');
let windowOpen = events.filter(browserEvents, e => e.type === 'load');
let windowClose = events.filter(browserEvents, e => e.type === 'close');

let close = events.merge([tabClose, windowClose]);
let activate = events.merge([windowOpen, tabSelect]);

on(activate, 'data', ({target}) => {
  let [window, tab] = isWindow(target)
                        ? [target, getActiveTab(target)]
                        : [getOwnerWindow(target), target];

  if (ignoreWindow(window)) return;

  for (let component of iterator(components)) {
    emit(stateEvents, 'data', {
      type: 'render',
      target: component,
      window: window,
      state: getStateFor(component, tab)
    });
  }
});

on(close, 'data', function({target}) {
  for (let component of iterator(components)) {
    components.get(component).delete(target);
  }
});
