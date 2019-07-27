


"use strict";

const system = require('sdk/system/events');
const { frames } = require('sdk/remote/child');


const EVENTS = {
  'content-document-interactive': 'ready',
  'chrome-document-interactive': 'ready',
  'content-document-loaded': 'load',
  'chrome-document-loaded': 'load',

}

function topicListener({ subject, type }) {
  let window = subject.defaultView;
  if (!window)
    return;
  let frame = frames.getFrameForWindow(subject.defaultView);
  if (frame)
    frame.port.emit('sdk/tab/event', EVENTS[type]);
}

for (let topic in EVENTS)
  system.on(topic, topicListener, true);


function eventListener({target, type, persisted}) {
  let frame = this;
  if (target === frame.content.document)
    frame.port.emit('sdk/tab/event', type, persisted);
}
frames.addEventListener('pageshow', eventListener, true);
