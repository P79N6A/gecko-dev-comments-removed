


"use strict";

const { utils: Cu, classes: Cc, interfaces: Ci } = Components;
const { Services } = Cu.import('resource://gre/modules/Services.jsm');

const cpmm = Cc['@mozilla.org/childprocessmessagemanager;1'].
             getService(Ci.nsISyncMessageSender);

this.EXPORTED_SYMBOLS = ["registerContentFrame"];



const PATH = __URI__.replace('framescript/content.jsm', '');

const { Loader } = Cu.import(PATH + 'toolkit/loader.js', {});


let addons = new Map();


cpmm.sendAsyncMessage('sdk/remote/process/start', {
  modulePath: PATH
});


cpmm.addMessageListener('sdk/remote/process/load', ({ data: { modulePath, loaderID, options, reason } }) => {
  if (modulePath != PATH)
    return;

  
  if (addons.has(loaderID))
    return;

  let loader = Loader.Loader(options);
  let addon = {
    loader,
    require: Loader.Require(loader, { id: 'LoaderHelper' }),
  }
  addons.set(loaderID, addon);

  cpmm.sendAsyncMessage('sdk/remote/process/attach', {
    loaderID,
    processID: Services.appinfo.processID,
    isRemote: Services.appinfo.processType != Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT
  });

  addon.child = addon.require('sdk/remote/child');

  for (let contentFrame of frames.values())
    addon.child.registerContentFrame(contentFrame);
});


cpmm.addMessageListener('sdk/remote/process/unload', ({ data: { loaderID, reason } }) => {
  if (!addons.has(loaderID))
    return;

  let addon = addons.get(loaderID);
  Loader.unload(addon.loader, reason);

  
  
  addons.set(loaderID, {});
})


let frames = new Set();

this.registerContentFrame = contentFrame => {
  contentFrame.addEventListener("unload", () => {
    unregisterContentFrame(contentFrame);
  }, false);

  frames.add(contentFrame);

  for (let addon of addons.values()) {
    if ("child" in addon)
      addon.child.registerContentFrame(contentFrame);
  }
};

function unregisterContentFrame(contentFrame) {
  frames.delete(contentFrame);

  for (let addon of addons.values()) {
    if ("child" in addon)
      addon.child.unregisterContentFrame(contentFrame);
  }
}
