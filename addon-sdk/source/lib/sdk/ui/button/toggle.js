


'use strict';

module.metadata = {
  'stability': 'experimental',
  'engines': {
    'Firefox': '> 28'
  }
};




try {
  require('chrome').Cu.import('resource:///modules/CustomizableUI.jsm', {});
}
catch (e) {
  throw Error('Unsupported Application: The module ' + module.id +
              ' does not support this application.');
}

const { Class } = require('../../core/heritage');
const { merge } = require('../../util/object');
const { Disposable } = require('../../core/disposable');
const { on, off, emit, setListeners } = require('../../event/core');
const { EventTarget } = require('../../event/target');

const view = require('./view');
const { toggleButtonContract, toggleStateContract } = require('./contract');
const { properties, render, state, register, unregister } = require('../state');
const { events: stateEvents } = require('../state/events');
const { events: viewEvents } = require('./view/events');
const events = require('../../event/utils');

const { id: addonID } = require('../../self');
const { identify } = require('../id');

const buttons = new Map();

const toWidgetId = id =>
  ('toggle-button--' + addonID.toLowerCase()+ '-' + id).
    replace(/[^a-z0-9_-]/g, '');

const ToggleButton = Class({
  extends: EventTarget,
  implements: [
    properties(toggleStateContract),
    state(toggleStateContract),
    Disposable
  ],
  setup: function setup(options) {
    let state = merge({
      disabled: false,
      checked: false
    }, toggleButtonContract(options));

    let id = toWidgetId(options.id);

    register(this, state);

    
    setListeners(this, options);

    buttons.set(id, this);

    view.create(merge({ type: 'checkbox' }, state, { id: id }));
  },

  dispose: function dispose() {
    let id = toWidgetId(this.id);
    buttons.delete(id);

    off(this);

    view.dispose(id);

    unregister(this);
  },

  get id() this.state().id,

  click: function click() view.click(toWidgetId(this.id))
});
exports.ToggleButton = ToggleButton;

identify.define(ToggleButton, ({id}) => toWidgetId(id));

let toggleButtonStateEvents = events.filter(stateEvents,
  e => e.target instanceof ToggleButton);

let toggleButtonViewEvents = events.filter(viewEvents,
  e => buttons.has(e.target));

let clickEvents = events.filter(toggleButtonViewEvents, e => e.type === 'click');
let updateEvents = events.filter(toggleButtonViewEvents, e => e.type === 'update');

on(toggleButtonStateEvents, 'data', ({target, window, state}) => {
  let id = toWidgetId(target.id);

  view.setIcon(id, window, state.icon);
  view.setLabel(id, window, state.label);
  view.setDisabled(id, window, state.disabled);
  view.setChecked(id, window, state.checked);
});

on(clickEvents, 'data', ({target: id, window}) => {
  let button = buttons.get(id);
  let state = button.state('tab');

  state = merge({}, state, { checked: !state.checked });

  button.state('tab', state);

  emit(button, 'click', state);

  emit(button, 'change', state);
});

on(updateEvents, 'data', ({target: id, window}) => {
  render(buttons.get(id), window);
});
