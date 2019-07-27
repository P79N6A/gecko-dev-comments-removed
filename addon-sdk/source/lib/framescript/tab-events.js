


"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
const observerSvc = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);


const EVENTS = {
  'content-document-interactive': 'ready',
  'chrome-document-interactive': 'ready',
  'content-document-loaded': 'load',
  'chrome-document-loaded': 'load',

}

function listener(subject, topic) {
  
  
  if (!docShell)
    observerSvc.removeObserver(listener, topic);
  else if (subject === content.document)
    sendAsyncMessage('sdk/tab/event', { type: EVENTS[topic] });
}

for (let topic in EVENTS)
  observerSvc.addObserver(listener, topic, false);


addEventListener('pageshow', ({ target, type, persisted }) => {
  if (target === content.document)
    sendAsyncMessage('sdk/tab/event', { type, persisted });
}, true);



let keepAlive = new Map();

addMessageListener('sdk/worker/create', ({ data: { options, addon }}) => {
  options.manager = this;
  let { loader } = Cu.import(addon.paths[''] + 'framescript/LoaderHelper.jsm', {});
  let { WorkerChild } = loader(addon).require('sdk/content/worker-child');
  sendAsyncMessage('sdk/worker/attach', { id: options.id });
  keepAlive.set(options.id, new WorkerChild(options));
})

addMessageListener('sdk/worker/event', ({ data: { id, args: [event]}}) => {
  if (event === 'detach')
    keepAlive.delete(id);
})
