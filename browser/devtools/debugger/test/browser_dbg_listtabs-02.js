





let testPage = ("data:text/html;charset=utf-8,"
                + encodeURIComponent("<title>JS Debugger BrowserTabList test page</title>" +
                                     "<body>Yo.</body>"));

let tabList;
let firstActor, actorA;
let tabA, tabB, tabC;
let newWin;

let onListChangedCount = 0;
function onListChangedHandler() {
  onListChangedCount++;
}

function test() {
  tabList = new DebuggerServer.BrowserTabList("fake DebuggerServerConnection");
  tabList._testing = true;
  tabList.onListChanged = onListChangedHandler;
  checkSingleTab(function () {
    is(onListChangedCount, 0, "onListChanged handler call count");
    tabA = addTab(testPage, onTabA);
  });
}

function checkSingleTab(callback) {
  tabList.getList().then(function (tabActors) {
    is(tabActors.length, 1, "initial tab list: contains initial tab");
    firstActor = tabActors[0];
    is(firstActor.url, "about:blank", "initial tab list: initial tab URL is 'about:blank'");
    is(firstActor.title, "New Tab", "initial tab list: initial tab title is 'New Tab'");
    callback();
  });
}

function onTabA() {
  is(onListChangedCount, 1, "onListChanged handler call count");

  tabList.getList().then(function (tabActors) {
    tabActors = new Set(tabActors);
    is(tabActors.size, 2, "tabA opened: two tabs in list");
    ok(tabActors.has(firstActor), "tabA opened: initial tab present");

    info("actors: " + [a.url for (a of tabActors)]);
    actorA = [a for (a of tabActors) if (a !== firstActor)][0];
    ok(actorA.url.match(/^data:text\/html;/), "tabA opened: new tab URL");
    is(actorA.title, "JS Debugger BrowserTabList test page", "tabA opened: new tab title");

    tabB = addTab(testPage, onTabB);
  });
}

function onTabB() {
  is(onListChangedCount, 2, "onListChanged handler call count");

  tabList.getList().then(function (tabActors) {
    tabActors = new Set(tabActors);
    is(tabActors.size, 3, "tabB opened: three tabs in list");

    
    gBrowser.tabContainer.addEventListener("TabClose", function onClose(aEvent) {
      gBrowser.tabContainer.removeEventListener("TabClose", onClose, false);
      ok(!aEvent.detail, "This was a normal tab close");
      
      executeSoon(testTabClose);
    }, false);
    gBrowser.removeTab(tabA);
  });
}

function testTabClose() {
  is(onListChangedCount, 3, "onListChanged handler call count");

  tabList.getList().then(function (tabActors) {
    tabActors = new Set(tabActors);
    is(tabActors.size, 2, "tabA closed: two tabs in list");
    ok(tabActors.has(firstActor), "tabA closed: initial tab present");

    info("actors: " + [a.url for (a of tabActors)]);
    actorA = [a for (a of tabActors) if (a !== firstActor)][0];
    ok(actorA.url.match(/^data:text\/html;/), "tabA closed: new tab URL");
    is(actorA.title, "JS Debugger BrowserTabList test page", "tabA closed: new tab title");

    
    tabC = addTab(testPage, onTabC);
  });
}

function onTabC() {
  is(onListChangedCount, 4, "onListChanged handler call count");

  tabList.getList().then(function (tabActors) {
    tabActors = new Set(tabActors);
    is(tabActors.size, 3, "tabC opened: three tabs in list");

    gBrowser.tabContainer.addEventListener("TabClose", function onClose2(aEvent) {
      gBrowser.tabContainer.removeEventListener("TabClose", onClose2, false);
      ok(aEvent.detail, "This was a tab closed by moving");
      
      executeSoon(testWindowClose);
    }, false);
    newWin = gBrowser.replaceTabWithWindow(tabC);
  });
}

function testWindowClose() {
  is(onListChangedCount, 5, "onListChanged handler call count");

  tabList.getList().then(function (tabActors) {
    tabActors = new Set(tabActors);
    is(tabActors.size, 3, "tabC closed: three tabs in list");
    ok(tabActors.has(firstActor), "tabC closed: initial tab present");

    info("actors: " + [a.url for (a of tabActors)]);
    actorA = [a for (a of tabActors) if (a !== firstActor)][0];
    ok(actorA.url.match(/^data:text\/html;/), "tabC closed: new tab URL");
    is(actorA.title, "JS Debugger BrowserTabList test page", "tabC closed: new tab title");

    
    newWin.addEventListener("unload", function onUnload(aEvent) {
      newWin.removeEventListener("unload", onUnload, false);
      ok(!aEvent.detail, "This was a normal window close");
      
      executeSoon(checkWindowClose);
    }, false);
    newWin.close();
  });
}

function checkWindowClose() {
  is(onListChangedCount, 6, "onListChanged handler call count");

  
  tabList.getList().then(function (tabActors) {
    tabActors = new Set(tabActors);
    is(tabActors.size, 2, "newWin closed: two tabs in list");
    ok(tabActors.has(firstActor), "newWin closed: initial tab present");

    info("actors: " + [a.url for (a of tabActors)]);
    actorA = [a for (a of tabActors) if (a !== firstActor)][0];
    ok(actorA.url.match(/^data:text\/html;/), "newWin closed: new tab URL");
    is(actorA.title, "JS Debugger BrowserTabList test page", "newWin closed: new tab title");

    
    gBrowser.tabContainer.addEventListener("TabClose", function onClose(aEvent) {
      gBrowser.tabContainer.removeEventListener("TabClose", onClose, false);
      ok(!aEvent.detail, "This was a normal tab close");
      
      executeSoon(finishTest);
    }, false);
    gBrowser.removeTab(tabB);
  });
}

function finishTest() {
  checkSingleTab(finish);
}
