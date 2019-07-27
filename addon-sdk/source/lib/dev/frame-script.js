



"use strict";
(function({content, sendSyncMessage, addMessageListener, sendAsyncMessage}) {

const Cc = Components.classes;
const Ci = Components.interfaces;
const observerService = Cc["@mozilla.org/observer-service;1"]
                        .getService(Ci.nsIObserverService);

const channels = new Map();
const handles = new WeakMap();





const demarshal = (handle) => {
  if (handle.type === "MessagePort") {
    if (!channels.has(handle.id)) {
      const channel = new content.MessageChannel();
      channels.set(handle.id, channel);
      handles.set(channel.port1, handle);
      channel.port1.onmessage = onOutPort;
    }
    return channels.get(handle.id).port2;
  }
  return null;
};

const onOutPort = event => {
  const handle = handles.get(event.target);
  sendAsyncMessage("sdk/port/message", {
    port: handle,
    message: event.data
  });
};

const onInPort = ({data}) => {
  const channel = channels.get(data.port.id);
  if (channel)
    channel.port1.postMessage(data.message);
};

const onOutEvent = event =>
  sendSyncMessage("sdk/event/" + event.type,
                  { type: event.type,
                    data: event.data });

const onInMessage = (message) => {
  const {type, data, origin, bubbles, cancelable, ports} = message.data;

  const event = new content.MessageEvent(type, {
    bubbles: bubbles,
    cancelable: cancelable,
    data: data,
    origin: origin,
    target: content,
    source: content,
    ports: ports.map(demarshal)
  });
  content.dispatchEvent(event);
};

const onReady = event => {
  channels.clear();
};

addMessageListener("sdk/event/message", onInMessage);
addMessageListener("sdk/port/message", onInPort);

const observer = {
  handleEvent: ({target, type}) => {
    observer.observe(target, type);
  },
  observe: (document, topic, data) => {
    
    
    
    
    
    if (!docShell) {
      observerService.removeObserver(observer, topic);
    }
    else if (document === content.document) {
      if (topic.endsWith("-document-interactive")) {
        sendAsyncMessage("sdk/event/ready", {
          type: "ready",
          readyState: document.readyState,
          uri: document.documentURI
        });
      }
      if (topic.endsWith("-document-loaded")) {
        sendAsyncMessage("sdk/event/load", {
          type: "load",
          readyState: document.readyState,
          uri: document.documentURI
        });
      }
      if (topic === "unload") {
        channels.clear();
        sendAsyncMessage("sdk/event/unload", {
          type: "unload",
          readyState: "uninitialized",
          uri: document.documentURI
        });
      }
    }
  }
};

observerService.addObserver(observer, "content-document-interactive", false);
observerService.addObserver(observer, "content-document-loaded", false);
observerService.addObserver(observer, "chrome-document-interactive", false);
observerService.addObserver(observer, "chrome-document-loaded", false);
addEventListener("unload", observer, false);

})(this);
