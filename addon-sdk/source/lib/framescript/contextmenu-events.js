


"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

const { Services } = Cu.import("resource://gre/modules/Services.jsm", {});


let keepAlive = new Map();





addMessageListener('sdk/contextmenu/createitems', ({ data: { items, addon }}) => {
  let { loader } = Cu.import(addon.paths[''] + 'framescript/LoaderHelper.jsm', {});

  for (let itemoptions of items) {
    let { RemoteItem } = loader(addon).require('sdk/content/context-menu');
    let item = new RemoteItem(itemoptions, this);

    let oldItem = keepAlive.get(item.id);
    if (oldItem) {
      oldItem.destroy();
    }

    keepAlive.set(item.id, item);
  }
});

addMessageListener('sdk/contextmenu/destroyitems', ({ data: { items }}) => {
  for (let id of items) {
    let item = keepAlive.get(id);
    item.destroy();
    keepAlive.delete(id);
  }
});

sendAsyncMessage('sdk/contextmenu/requestitems');

Services.obs.addObserver(function(subject, topic, data) {
  
  
  let { event: { target: popupNode }, addonInfo } = subject.wrappedJSObject;
  if (popupNode.ownerDocument.defaultView.top != content)
    return;

  for (let item of keepAlive.values()) {
    item.getContextState(popupNode, addonInfo);
  }
}, "content-contextmenu", false);

addMessageListener('sdk/contextmenu/activateitems', ({ data: { items, data }, objects: { popupNode }}) => {
  for (let id of items) {
    let item = keepAlive.get(id);
    if (!item)
      continue;

    item.activate(popupNode, data);
  }
});
