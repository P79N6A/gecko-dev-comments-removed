





const ADDON3_URL = EXAMPLE_URL + "addon3.xpi";

let gAddon, gClient, gThreadClient, gDebugger, gSources, gTitle;

function onMessage(event) {
  try {
    let json = JSON.parse(event.data);
    switch (json.name) {
      case "toolbox-title":
        gTitle = json.data.value;
        break;
    }
  } catch(e) { Cu.reportError(e); }
}

function test() {
  Task.spawn(function () {
    if (!DebuggerServer.initialized) {
      DebuggerServer.init(() => true);
      DebuggerServer.addBrowserActors();
    }

    gBrowser.selectedTab = gBrowser.addTab();
    let iframe = document.createElement("iframe");
    document.documentElement.appendChild(iframe);

    window.addEventListener("message", onMessage);

    let transport = DebuggerServer.connectPipe();
    gClient = new DebuggerClient(transport);

    let connected = promise.defer();
    gClient.connect(connected.resolve);
    yield connected.promise;

    yield installAddon();
    let debuggerPanel = yield initAddonDebugger(gClient, ADDON3_URL, iframe);
    gDebugger = debuggerPanel.panelWin;
    gThreadClient = gDebugger.gThreadClient;
    gSources = gDebugger.DebuggerView.Sources;

    yield testSources();
    yield uninstallAddon();
    yield closeConnection();
    yield debuggerPanel._toolbox.destroy();
    iframe.remove();
    window.removeEventListener("message", onMessage);
    finish();
  });
}

function installAddon () {
  return addAddon(ADDON3_URL).then(aAddon => {
    gAddon = aAddon;
  });
}

function testSources() {
  let deferred = promise.defer();
  let foundAddonModule = false;
  let foundSDKModule = 0;
  let foundAddonBootstrap = false;

  gThreadClient.getSources(({sources}) => {
    ok(sources.length, "retrieved sources");

    for (let source of sources) {
      let url = source.url.split(" -> ").pop();
      info(source.url + "\n\n\n" + url);
      let { label, group } = gSources.getItemByValue(source.url).attachment;

      if (url.indexOf("resource://gre/modules/commonjs/") === 0) {
        is(label, url.substring(32), "correct truncated label");
        is(group, "Add-on SDK", "correct SDK group");
        foundSDKModule++;
      } else if (url.indexOf("resource://gre/modules/commonjs/method") === 0) {
        is(label.indexOf("method/"), 0, "correct truncated label");
        is(group, "Add-on SDK", "correct SDK group");
        foundSDKModule++;
      } else if (url.indexOf("resource://jid1-ami3akps3baaeg-at-jetpack") === 0) {
        is(label, "main.js", "correct label for addon code");
        is(group, "resource://jid1-ami3akps3baaeg-at-jetpack", "addon code is in its own group");
        foundAddonModule = true;
      } else if (url.endsWith("/jid1-ami3akps3baaeg@jetpack.xpi!/bootstrap.js")) {
        is(label, "bootstrap.js", "correct label for bootstrap script");
        is(group, "jar:", "bootstrap script is in its own group");
        foundAddonBootstrap = true;
      } else {
        ok(false, "Saw an unexpected source: " + url);
      }
    }

    ok(foundAddonModule, "found code for the addon in the list");
    ok(foundAddonBootstrap, "found bootstrap for the addon in the list");
    
    
    ok(foundSDKModule > 10, "SDK modules are listed");

    is(gTitle, "Debugger - browser_dbg_addon3", "Saw the right toolbox title.");

    deferred.resolve();
  });

  return deferred.promise;
}

function uninstallAddon() {
  return removeAddon(gAddon);
}

function closeConnection () {
  let deferred = promise.defer();
  gClient.close(deferred.resolve);
  return deferred.promise;
}

registerCleanupFunction(function() {
  gClient = null;
  gAddon = null;
  gThreadClient = null;
  gDebugger = null;
  gSources = null;
  while (gBrowser.tabs.length > 1)
    gBrowser.removeCurrentTab();
});
