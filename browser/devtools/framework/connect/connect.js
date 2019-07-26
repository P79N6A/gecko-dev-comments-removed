





"use strict";

const Cu = Components.utils;
Cu.import("resource:///modules/devtools/Target.jsm");
Cu.import("resource:///modules/devtools/Toolbox.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");
Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");

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

}, true);




function submit() {
  
  document.body.classList.add("connecting");

  
  let host = document.getElementById("host").value;
  Services.prefs.setCharPref("devtools.debugger.remote-host", host);

  let port = document.getElementById("port").value;
  Services.prefs.setIntPref("devtools.debugger.remote-port", port);

  
  let transport = debuggerSocketConnect(host, port);
  gClient = new DebuggerClient(transport);
  let delay = Services.prefs.getIntPref("devtools.debugger.remote-timeout");
  gConnectionTimeout = setTimeout(handleConnectionTimeout, delay);
  gClient.connect(onConnectionReady);
}




function onConnectionReady(aType, aTraits) {
  clearTimeout(gConnectionTimeout);
  gClient.listTabs(function(aResponse) {
    document.body.classList.remove("connecting");
    document.body.classList.add("actors-mode");

    let parent = document.getElementById("tabActors");

    
    let globals = JSON.parse(JSON.stringify(aResponse));
    delete globals.tabs;
    delete globals.selected;
    
    

    
    for (let i = 0; i < aResponse.tabs.length; i++) {
      buildLink(aResponse.tabs[i], parent, i == aResponse.selected);
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

    
    let firstLink = parent.querySelector("a:first-of-type");
    if (firstLink) {
      firstLink.focus();
    }

  });
}




function buildLink(tab, parent, selected) {
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





function openToolbox(form, chrome=false) {
  let options = {
    form: form,
    client: gClient,
    chrome: chrome
  };
  let target = TargetFactory.forTab(options);
  target.makeRemote(options).then(function() {
    gDevTools.showToolbox(target, "webconsole", Toolbox.HostType.WINDOW);
    window.close();
  });
}
