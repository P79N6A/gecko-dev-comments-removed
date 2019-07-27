



'use strict';

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

XPCOMUtils.defineLazyServiceGetter(
  this,
  "PushNotificationService",
  "@mozilla.org/push/NotificationService;1",
  "nsIPushNotificationService"
);

const bundle = Services.strings.createBundle(
  "chrome://global/locale/aboutServiceWorkers.properties");

let gSWM;
let gSWCount = 0;

function init() {
  let enabled = Services.prefs.getBoolPref("dom.serviceWorkers.enabled");
  if (!enabled) {
    let div = document.getElementById("warning_not_enabled");
    div.classList.add("active");
    return;
  }

  gSWM = Cc["@mozilla.org/serviceworkers/manager;1"]
           .getService(Ci.nsIServiceWorkerManager);
  if (!gSWM) {
    dump("AboutServiceWorkers: Failed to get the ServiceWorkerManager service!\n");
    return;
  }

  let data = gSWM.getAllRegistrations();
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

  PushNotificationService.registration(info.scope).then(
    pushRecord => {
      createItem(bundle.GetStringFromName('pushEndpoint'), JSON.stringify(pushRecord));
    },
    error => {
      dump("about:serviceworkers - push registration failed\n");
    }
  );

  let updateButton = document.createElement("button");
  updateButton.appendChild(document.createTextNode(bundle.GetStringFromName('update')));
  updateButton.onclick = function() {
    gSWM.update(info.scope);
  };
  div.appendChild(updateButton);

  let unregisterButton = document.createElement("button");
  unregisterButton.appendChild(document.createTextNode(bundle.GetStringFromName('unregister')));
  div.appendChild(unregisterButton);

  let loadingMessage = document.createElement('span');
  loadingMessage.appendChild(document.createTextNode(bundle.GetStringFromName('waiting')));
  loadingMessage.classList.add('inactive');
  div.appendChild(loadingMessage);

  unregisterButton.onclick = function() {
    let cb = {
      unregisterSucceeded: function() {
        parent.removeChild(div);

        if (!--gSWCount) {
         let div = document.getElementById("warning_no_serviceworkers");
         div.classList.add("active");
        }
      },

      unregisterFailed: function() {
        alert(bundle.GetStringFromName('unregisterError'));
      },

      QueryInterface: XPCOMUtils.generateQI([Ci.nsIServiceWorkerUnregisterCallback])
    };

    loadingMessage.classList.remove('inactive');
    gSWM.unregister(info.principal, cb, info.scope);
  };

  let sep = document.createElement('hr');
  div.appendChild(sep);

  ++gSWCount;
}

window.addEventListener("DOMContentLoaded", function load() {
  window.removeEventListener("DOMContentLoaded", load);
  init();
});
