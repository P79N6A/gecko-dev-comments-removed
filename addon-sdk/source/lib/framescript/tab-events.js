


"use strict";

const observerSvc = Components.classes["@mozilla.org/observer-service;1"].
                    getService(Components.interfaces.nsIObserverService);


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
