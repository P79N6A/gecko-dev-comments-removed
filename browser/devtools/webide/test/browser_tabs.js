

"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webide/test/doc_tabs.html";

function test() {
  waitForExplicitFinish();
  SimpleTest.requestCompleteLog();

  Task.spawn(function() {
    const { DebuggerServer } =
      Cu.import("resource://gre/modules/devtools/dbg-server.jsm", {});

    
    
    DebuggerServer.destroy();
    DebuggerServer.init();
    DebuggerServer.addBrowserActors();

    let tab = yield addTab(TEST_URI);

    let win = yield openWebIDE();

    yield connectToLocal(win);

    is(Object.keys(DebuggerServer._connections).length, 1, "Locally connected");

    yield selectTabProject(win);

    let project = win.AppManager.selectedProject;
    is(project.location, TEST_URI, "Location is correct");
    is(project.name, "example.com: Test Tab", "Name is correct");

    
    let tabsNode = win.document.querySelector("#project-panel-tabs");
    is(tabsNode.querySelectorAll(".panel-item").length, 2, "2 tabs available");
    yield removeTab(tab);
    yield waitForUpdate(win, "runtime-targets");
    is(tabsNode.querySelectorAll(".panel-item").length, 1, "1 tab available");

    yield win.Cmds.disconnectRuntime();
    yield closeWebIDE(win);

    DebuggerServer.destroy();
  }).then(finish, handleError);
}

function connectToLocal(win) {
  let deferred = promise.defer();
  win.AppManager.connection.once(
      win.Connection.Events.CONNECTED,
      () => deferred.resolve());
  win.document.querySelectorAll(".runtime-panel-item-other")[1].click();
  return deferred.promise;
}

function selectTabProject(win) {
  return Task.spawn(function() {
    yield win.Cmds.showProjectPanel();
    yield waitForUpdate(win, "runtime-targets");
    let tabsNode = win.document.querySelector("#project-panel-tabs");
    let tabNode = tabsNode.querySelectorAll(".panel-item")[1];
    tabNode.click();
  });
}
