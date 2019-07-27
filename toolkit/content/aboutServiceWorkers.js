



'use strict';

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Services.jsm");

const bundle = Services.strings.createBundle(
  "chrome://global/locale/aboutServiceWorkers.properties");

function init() {
  let enabled = Services.prefs.getBoolPref("dom.serviceWorkers.enabled");
  if (!enabled) {
    let div = document.getElementById("warning_not_enabled");
    div.classList.add("active");
    return;
  }

  let swm = Cc["@mozilla.org/serviceworkers/manager;1"]
              .getService(Ci.nsIServiceWorkerManager);
  if (!swm) {
    dump("AboutServiceWorkers: Failed to get the ServiceWorkerManager service!\n");
    return;
  }

  let data = swm.getAllRegistrations();
  if (!data) {
    dump("AboutServiceWorkers: Failed to retrieve the registrations.\n");
    return;
  }

  let length = data.length;
  if (!length) {
    let div = document.getElementById("warning_no_serviceworkers");
    div.classList.add("active");
    return;
  }

  for (let i = 0; i < length; ++i) {
    let info = data.queryElementAt(i, Ci.nsIServiceWorkerInfo);
    if (!info) {
      dump("AboutServiceWorkers: Invalid nsIServiceWorkerInfo interface.\n");
      continue;
    }

    display(info);
  }
}

function display(info) {
  let parent = document.getElementById("serviceworkers");

  let div = document.createElement('div');
  parent.appendChild(div);

  let title = document.createElement('h2');
  let titleStr = bundle.formatStringFromName('title', [info.principal.origin], 1);
  title.appendChild(document.createTextNode(titleStr));
  div.appendChild(title);

  if (info.principal.appId) {
    let b2gtitle = document.createElement('h3');
    let trueFalse = bundle.GetStringFromName(info.principal.isInBrowserElement ? 'true' : 'false');
    let b2gtitleStr = bundle.formatStringFromName('b2gtitle', [info.principal.appId, trueFalse], 2);
    b2gtitle.appendChild(document.createTextNode(b2gtitleStr));
    div.appendChild(b2gtitle);
  }

  let list = document.createElement('ul');
  div.appendChild(list);

  function createItem(title, value, makeLink) {
    let item = document.createElement('li');
    list.appendChild(item);

    let bold = document.createElement('strong');
    bold.appendChild(document.createTextNode(title + " "));
    item.appendChild(bold);

    if (makeLink) {
      let link = document.createElement("a");
      link.href = value;
      link.target = "_blank";
      link.appendChild(document.createTextNode(value));
      item.appendChild(link);
    } else {
      item.appendChild(document.createTextNode(value));
    }
  }

  createItem(bundle.GetStringFromName('scope'), info.scope);
  createItem(bundle.GetStringFromName('scriptSpec'), info.scriptSpec, true);
  createItem(bundle.GetStringFromName('currentWorkerURL'), info.currentWorkerURL, true);
  createItem(bundle.GetStringFromName('activeCacheName'), info.activeCacheName);
  createItem(bundle.GetStringFromName('waitingCacheName'), info.waitingCacheName);

  let sep = document.createElement('hr');
  div.appendChild(sep);
}

window.addEventListener("DOMContentLoaded", function load() {
  window.removeEventListener("DOMContentLoaded", load);
  init();
});
