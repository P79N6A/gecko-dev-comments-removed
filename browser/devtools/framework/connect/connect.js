





"use strict";

const Cu = Components.utils;
Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");
let {gDevTools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});

let gClient;
let gConnectionTimeout;

XPCOMUtils.defineLazyGetter(window, 'l10n', function () {
  return Services.strings.createBundle('chrome://browser/locale/devtools/connection-screen.properties');
});





window.addEventListener("DOMContentLoaded", function onDOMReady() {
  window.removeEventListener("DOMContentLoaded", onDOMReady, true);
  let host = Services.prefs.getCharPref("devtools.debugger.remote-host");
  let port = Services.prefs.getIntPref("devtools.debugger.remote-port");

  if (host) {
    document.getElementById("host").value = host;
  }

  if (port) {
    document.getElementById("port").value = port;
  }

  let form = document.querySelector("#connection-form form");
  form.addEventListener("submit", function() {
    window.submit().catch(e => {
      Cu.reportError(e);
      
      showError("unexpected");
    });
  });
}, true);




let submit = Task.async(function*() {
  
  document.body.classList.add("connecting");

  let host = document.getElementById("host").value;
  let port = document.getElementById("port").value;

  
  try {
    Services.prefs.setCharPref("devtools.debugger.remote-host", host);
    Services.prefs.setIntPref("devtools.debugger.remote-port", port);
  } catch(e) {
    
  }

  
  let transport = yield DebuggerClient.socketConnect({ host, port });
  gClient = new DebuggerClient(transport);
  let delay = Services.prefs.getIntPref("devtools.debugger.remote-timeout");
  gConnectionTimeout = setTimeout(handleConnectionTimeout, delay);
  let response = yield clientConnect();
  yield onConnectionReady(...response);
});

function clientConnect() {
  let deferred = promise.defer();
  gClient.connect((...args) => deferred.resolve(args));
  return deferred.promise;
}




let onConnectionReady = Task.async(function*(aType, aTraits) {
  clearTimeout(gConnectionTimeout);

  let deferred = promise.defer();
  gClient.listAddons(deferred.resolve);
  let response = yield deferred.promise;

  let parent = document.getElementById("addonActors")
  if (!response.error && response.addons.length > 0) {
    
    for (let addon of response.addons) {
      if (!addon.debuggable) {
        continue;
      }
      buildAddonLink(addon, parent);
    }
  }
  else {
    
    parent.previousElementSibling.remove();
    parent.remove();
  }

  deferred = promise.defer();
  gClient.listTabs(deferred.resolve);
  response = yield deferred.promise;

  parent = document.getElementById("tabActors");

  
  let globals = Cu.cloneInto(response, {});
  delete globals.tabs;
  delete globals.selected;
  
  

  
  for (let i = 0; i < response.tabs.length; i++) {
    buildTabLink(response.tabs[i], parent, i == response.selected);
  }

  let gParent = document.getElementById("globalActors");

  
  if (Object.keys(globals).length > 1) {
    let a = document.createElement("a");
    a.onclick = function() {
      openToolbox(globals, true);

    }
    a.title = a.textContent = window.l10n.GetStringFromName("mainProcess");
    a.className = "remote-process";
    a.href = "#";
    gParent.appendChild(a);
  }
  
  let selectedLink = parent.querySelector("a.selected");
  if (selectedLink) {
    parent.insertBefore(selectedLink, parent.firstChild);
  }

  document.body.classList.remove("connecting");
  document.body.classList.add("actors-mode");

  
  let firstLink = parent.querySelector("a:first-of-type");
  if (firstLink) {
    firstLink.focus();
  }
});




function buildAddonLink(addon, parent) {
  let a = document.createElement("a");
  a.onclick = function() {
    openToolbox(addon, true, "jsdebugger", false);
  }

  a.textContent = addon.name;
  a.title = addon.id;
  a.href = "#";

  parent.appendChild(a);
}




function buildTabLink(tab, parent, selected) {
  let a = document.createElement("a");
  a.onclick = function() {
    openToolbox(tab);
  }

  a.textContent = tab.title;
  a.title = tab.url;
  if (!a.textContent) {
    a.textContent = tab.url;
  }
  a.href = "#";

  if (selected) {
    a.classList.add("selected");
  }

  parent.appendChild(a);
}




function showError(type) {
  document.body.className = "error";
  let activeError = document.querySelector(".error-message.active");
  if (activeError) {
    activeError.classList.remove("active");
  }
  activeError = document.querySelector(".error-" + type);
  if (activeError) {
    activeError.classList.add("active");
  }
}




function handleConnectionTimeout() {
  showError("timeout");
}





function openToolbox(form, chrome=false, tool="webconsole", isTabActor) {
  let options = {
    form: form,
    client: gClient,
    chrome: chrome,
    isTabActor: isTabActor
  };
  devtools.TargetFactory.forRemoteTab(options).then((target) => {
    let hostType = devtools.Toolbox.HostType.WINDOW;
    gDevTools.showToolbox(target, tool, hostType).then((toolbox) => {
      toolbox.once("destroyed", function() {
        gClient.close();
      });
    }, console.error.bind(console));
    window.close();
  }, console.error.bind(console));
}
