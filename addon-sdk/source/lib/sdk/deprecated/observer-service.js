



"use strict";

module.metadata = {
  "stability": "deprecated"
};

const { Cc, Ci } = require("chrome");
const { when: unload } = require("../system/unload");
const { ns } = require("../core/namespace");
const { on, off, emit, once } = require("../system/events");
const { id } = require("../self");

const subscribers = ns();
const cache = [];










exports.topics = {
  



  APPLICATION_READY: id + "_APPLICATION_READY"
};

function Listener(callback, target) {
  return function listener({ subject, data }) {
    callback.call(target || callback, subject, data);
  }
}
















function add(topic, callback, target) {
  let listeners = subscribers(callback);
  if (!(topic in listeners)) {
    let listener = Listener(callback, target);
    listeners[topic] = listener;

    
    if (!~cache.indexOf(callback))
      cache.push(callback);

    on(topic, listener);
  }
};
exports.add = add;













function remove(topic, callback, target) {
  let listeners = subscribers(callback);
  if (topic in listeners) {
    let listener = listeners[topic];
    delete listeners[topic];

    
    
    let index = cache.indexOf(callback);
    if (~index && !Object.keys(listeners).length)
      cache.splice(index, 1)

    off(topic, listener);
  }
};
exports.remove = remove;

















function notify(topic, subject, data) {
  emit(topic, {
    subject: subject === undefined ? null : subject,
    data: data === undefined ? null : data
  });
}
exports.notify = notify;

unload(function() {
  
  
  cache.slice().forEach(function(callback) {
    Object.keys(subscribers(callback)).forEach(function(topic) {
      remove(topic, callback);
    });
  });
})
