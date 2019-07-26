


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
const { buttonContract, stateContract } = require('./contract');
const { properties, render, state, register, unregister } = require('../state');
const { events: stateEvents } = require('../state/events');
const { events: viewEvents } = require('./view/events');
const events = require('../../event/utils');

const buttons = new Map();

const ActionButton = Class({
  extends: EventTarget,
  implements: [
    properties(stateContract),
    state(stateContract),
    Disposable
  ],
  setup: function setup(options) {
    let state = merge({
      disabled: false
    }, buttonContract(options));

    register(this, state);

    
    setListeners(this, options);

    buttons.set(options.id, this);

    view.create(state);
  },

  dispose: function dispose() {
    buttons.delete(this.id);

    off(this);

    view.dispose(this.id);

    unregister(this);
  },

  get id() this.state().id,

  click: function click() { view.click(this.id) }
});
exports.ActionButton = ActionButton;

let actionButtonStateEvents = events.filter(stateEvents,
  e => e.target instanceof ActionButton);

let actionButtonViewEvents = events.filter(viewEvents,
  e => buttons.has(e.target));

let clickEvents = events.filter(actionButtonViewEvents, e => e.type === 'click');
let updateEvents = events.filter(actionButtonViewEvents, e => e.type === 'update');

on(clickEvents, 'data', ({target: id, window}) => {
  let button = buttons.get(id);
  let state = button.state('tab');

  emit(button, 'click', state);
});

on(updateEvents, 'data', ({target: id, window}) => {
  render(buttons.get(id), window);
});

on(actionButtonStateEvents, 'data', ({target, window, state}) => {
  let { id } = target;
  view.setIcon(id, window, state.icon);
  view.setLabel(id, window, state.label);
  view.setDisabled(id, window, state.disabled);
});
