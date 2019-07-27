


"use strict";

const { utils: Cu, classes: Cc, interfaces: Ci } = Components;
const { Loader } = Cu.import('resource://gre/modules/commonjs/toolkit/loader.js', {});
const cpmm = Cc['@mozilla.org/childprocessmessagemanager;1'].getService(Ci.nsISyncMessageSender);


let addons = new Map();

cpmm.addMessageListener('sdk/loader/unload', ({ data: options }) => {
  let key = JSON.stringify(options);
  let addon = addons.get(key);
  if (addon)
    addon.loader.unload();
  addons.delete(key);
})


function loader(options) {
  let key = JSON.stringify(options);
  let addon = addons.get(key) || {};
  if (!addon.loader) {
    addon.loader = Loader.Loader(options);
    addon.require = Loader.Require(addon.loader, { id: 'LoaderHelper' });
    addons.set(key, addon);
  }
  return addon;
}

const EXPORTED_SYMBOLS = ['loader'];
