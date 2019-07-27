


'use strict';

const { merge } = require('../util/object');
const { Class } = require('../core/heritage');
const { emit } = require('../event/core');
const { EventTarget } = require('../event/target');
const { getInnerId, getByInnerId } = require('../window/utils');
const { instanceOf, isObject } = require('../lang/type');
const system = require('../system/events');
const { when } = require('../system/unload');
const { WorkerSandbox } = require('./sandbox');
const { Ci } = require('chrome');
const { process, frames } = require('../remote/child');

const EVENTS = {
  'chrome-page-shown': 'pageshow',
  'content-page-shown': 'pageshow',
  'chrome-page-hidden': 'pagehide',
  'content-page-hidden': 'pagehide',
  'inner-window-destroyed': 'detach',
}




const WorkerChild = Class({
  implements: [EventTarget],

  initialize(options) {
    merge(this, options);
    keepAlive.set(this.id, this);

    this.windowId = getInnerId(this.window);

    this.port = EventTarget();
    this.port.on('*', this.send.bind(this, 'event'));
    this.on('*', this.send.bind(this));

    this.observe = this.observe.bind(this);

    for (let topic in EVENTS)
      system.on(topic, this.observe);

    this.receive = this.receive.bind(this);
    process.port.on('sdk/worker/message', this.receive);

    this.sandbox = WorkerSandbox(this, this.window);

    this.frozen = false;
    this.frozenMessages = [];
    this.on('pageshow', () => {
      this.frozen = false;
      this.frozenMessages.forEach(args => this.receive(null, this.id, args));
      this.frozenMessages = [];
    });
    this.on('pagehide', () => {
      this.frozen = true;
    });
  },

  
  receive(process, id, args) {
    if (id !== this.id)
      return;

    if (this.frozen)
      this.frozenMessages.push(args);
    else
      this.sandbox.emit(...args);

    if (args[0] === 'detach')
      this.destroy(args[1]);
  },

  send(...args) {
    args = JSON.parse(JSON.stringify(args, exceptions));
    process.port.emit('sdk/worker/event', this.id, args);
  },

  
  observe({ type, subject }) {
    if (!this.sandbox)
      return;

    if (subject.defaultView && getInnerId(subject.defaultView) === this.windowId) {
      this.sandbox.emitSync(EVENTS[type]);
      emit(this, EVENTS[type]);
    }

    if (type === 'inner-window-destroyed' &&
        subject.QueryInterface(Ci.nsISupportsPRUint64).data === this.windowId) {
      this.destroy();
    }
  },

  get frame() {
    return frames.getFrameForWindow(this.window.top);
  },

  
  destroy(reason) {
    if (!this.sandbox)
      return;

    for (let topic in EVENTS)
      system.off(topic, this.observe);
    process.port.off('sdk/worker/message', this.receive);

    this.sandbox.destroy(reason);
    this.sandbox = null;
    keepAlive.delete(this.id);

    this.send('detach');
  }
})
exports.WorkerChild = WorkerChild;


function exceptions(key, value) {
  if (!isObject(value) || !instanceOf(value, Error))
    return value;
  let _errorType = value.constructor.name;
  let { message, fileName, lineNumber, stack, name } = value;
  return { _errorType, message, fileName, lineNumber, stack, name };
}


let keepAlive = new Map();

process.port.on('sdk/worker/create', (process, options) => {
  options.window = getByInnerId(options.window);
  if (!options.window)
    return;

  let worker = new WorkerChild(options);
});

when(reason => {
  for (let worker of keepAlive.values())
    worker.destroy(reason);
});
