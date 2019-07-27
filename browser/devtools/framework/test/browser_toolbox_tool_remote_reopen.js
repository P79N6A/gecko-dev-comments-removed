







thisTestLeaksUncaughtRejectionsAndShouldBeFixed("Error: Shader Editor is still waiting for a WebGL context to be created.");

const { DebuggerServer } =
  Cu.import("resource://gre/modules/devtools/dbg-server.jsm", {});
const { DebuggerClient } =
  Cu.import("resource://gre/modules/devtools/dbg-client.jsm", {});



























function runTools(target) {
  return Task.spawn(function() {
    let toolIds = gDevTools.getToolDefinitionArray()
                           .filter(def => def.isTargetSupported(target))
                           .map(def => def.id);

    let toolbox;
    for (let index = 0; index < toolIds.length; index++) {
      let toolId = toolIds[index];

      info("About to open " + index + "/" + toolId);
      toolbox = yield gDevTools.showToolbox(target, toolId, "window");
      ok(toolbox, "toolbox exists for " + toolId);
      is(toolbox.currentToolId, toolId, "currentToolId should be " + toolId);

      let panel = toolbox.getCurrentPanel();
      ok(panel.isReady, toolId + " panel should be ready");
    }

    yield toolbox.destroy();
  });
}

function getClient() {
  let deferred = promise.defer();

  if (!DebuggerServer.initialized) {
    DebuggerServer.init();
    DebuggerServer.addBrowserActors();
  }

  let transport = DebuggerServer.connectPipe();
  let client = new DebuggerClient(transport);

  client.connect(() => {
    deferred.resolve(client);
  });

  return deferred.promise;
}

function getTarget(client) {
  let deferred = promise.defer();

  let tabList = client.listTabs(tabList => {
    let target = TargetFactory.forRemoteTab({
      client: client,
      form: tabList.tabs[tabList.selected],
      chrome: false
    });
    deferred.resolve(target);
  });

  return deferred.promise;
}

function test() {
  Task.spawn(function() {
    toggleAllTools(true);
    yield addTab("about:blank");

    let client = yield getClient();
    let target = yield getTarget(client);
    yield runTools(target);

    
    
    for (let pool of client.__pools) {
      if (!pool.__poolMap) {
        continue;
      }
      for (let actor of pool.__poolMap.keys()) {
        
        
        
        if (actor.contains("framerateActor")) {
          todo(false, "Front for " + actor + " still held in pool!");
          continue;
        }
        
        if (actor.contains("gcliActor")) {
          continue;
        }
        ok(false, "Front for " + actor + " still held in pool!");
      }
    }

    gBrowser.removeCurrentTab();
    DebuggerServer.destroy();
    toggleAllTools(false);
    finish();
  }, console.error);
}
