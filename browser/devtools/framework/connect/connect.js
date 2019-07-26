





"use strict";

const Cu = Components.utils;
Cu.import("resource:///modules/devtools/Target.jsm");
Cu.import("resource:///modules/devtools/Toolbox.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");

let gClient;

function submit() {
  document.body.classList.add("connecting");

  let host = document.getElementById("host").value;
  let port = document.getElementById("port").value;
  if (!host) {
    host = Services.prefs.getCharPref("devtools.debugger.remote-host");
  } else {
    Services.prefs.setCharPref("devtools.debugger.remote-host", host);
  }
  if (!port) {
    port = Services.prefs.getIntPref("devtools.debugger.remote-port");
  } else {
    Services.prefs.setIntPref("devtools.debugger.remote-port", port);
  }

  let transport = debuggerSocketConnect(host, port);
  let client = gClient = new DebuggerClient(transport);

  client.connect(function(aType, aTraits) {
    client.listTabs(function(aResponse) {
      document.body.classList.remove("connecting");
      document.body.classList.add("actors-mode");

      let parent = document.getElementById("actors");
      let focusSet = false;

      
      let globals = JSON.parse(JSON.stringify(aResponse));
      delete globals.tabs;
      delete globals.selected;
      
      
      if (Object.keys(globals).length > 1) {
        let a = document.createElement("a");
        a.onclick = function() {
          connect(globals, true);
        }

        a.title = a.textContent = "Remote process";
        a.href = "#";

        parent.appendChild(a);
      }

      
      if (aResponse.tabs.length > 0) {
        let header = document.createElement("div");
        header.innerHTML = "Tabs:";
        parent.appendChild(header);
      }
      for (let i = 0; i < aResponse.tabs.length; i++) {
        let tab = aResponse.tabs[i];

        let a = document.createElement("a");
        a.onclick = function() {
          connect(tab);
        }

        a.title = a.textContent = tab.title;
        a.href = "#";

        if (i == aResponse.selected) {
          a.title += " [*]";
          a.textContent = a.title;
        }

        parent.appendChild(a);

        if (!focusSet) {
          a.focus();
          focusSet = true;
        }
      }
    });
  });
}

function connect(form, chrome=false) {
  let target = TargetFactory.forRemote(form, gClient, chrome);
  gDevTools.showToolbox(target, "webconsole", Toolbox.HostType.WINDOW);
  window.close();
}
