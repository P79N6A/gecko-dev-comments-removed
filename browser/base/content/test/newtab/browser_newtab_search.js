





const ENGINE_NO_LOGO = "searchEngineNoLogo.xml";
const ENGINE_1X_LOGO = "searchEngine1xLogo.xml";
const ENGINE_2X_LOGO = "searchEngine2xLogo.xml";
const ENGINE_1X_2X_LOGO = "searchEngine1x2xLogo.xml";
const ENGINE_SUGGESTIONS = "searchSuggestionEngine.xml";

const SERVICE_EVENT_NAME = "ContentSearchService";

const LOGO_1X_DPI_SIZE = [65, 26];
const LOGO_2X_DPI_SIZE = [130, 52];









var gExpectedSearchEventQueue = [];

var gNewEngines = [];

function runTests() {
  let oldCurrentEngine = Services.search.currentEngine;

  yield addNewTabPageTab();

  
  
  info("Adding search event listener");
  getContentWindow().addEventListener(SERVICE_EVENT_NAME, searchEventListener);

  let panel = searchPanel();
  is(panel.state, "closed", "Search panel should be closed initially");

  
  
  panel.setAttribute("animate", "false");

  
  let noLogoEngine = null;
  yield promiseNewSearchEngine(ENGINE_NO_LOGO, 0).then(engine => {
    noLogoEngine = engine;
    TestRunner.next();
  });
  ok(!noLogoEngine.getIconURLBySize(...LOGO_1X_DPI_SIZE),
     "Sanity check: engine should not have 1x logo");
  ok(!noLogoEngine.getIconURLBySize(...LOGO_2X_DPI_SIZE),
     "Sanity check: engine should not have 2x logo");
  Services.search.currentEngine = noLogoEngine;
  yield promiseSearchEvents(["CurrentEngine"]).then(TestRunner.next);
  yield checkCurrentEngine(ENGINE_NO_LOGO, false, false);

  
  let logo1xEngine = null;
  yield promiseNewSearchEngine(ENGINE_1X_LOGO, 1).then(engine => {
    logo1xEngine = engine;
    TestRunner.next();
  });
  ok(!!logo1xEngine.getIconURLBySize(...LOGO_1X_DPI_SIZE),
     "Sanity check: engine should have 1x logo");
  ok(!logo1xEngine.getIconURLBySize(...LOGO_2X_DPI_SIZE),
     "Sanity check: engine should not have 2x logo");
  Services.search.currentEngine = logo1xEngine;
  yield promiseSearchEvents(["CurrentEngine"]).then(TestRunner.next);
  yield checkCurrentEngine(ENGINE_1X_LOGO, true, false);

  
  let logo2xEngine = null;
  yield promiseNewSearchEngine(ENGINE_2X_LOGO, 1).then(engine => {
    logo2xEngine = engine;
    TestRunner.next();
  });
  ok(!logo2xEngine.getIconURLBySize(...LOGO_1X_DPI_SIZE),
     "Sanity check: engine should not have 1x logo");
  ok(!!logo2xEngine.getIconURLBySize(...LOGO_2X_DPI_SIZE),
     "Sanity check: engine should have 2x logo");
  Services.search.currentEngine = logo2xEngine;
  yield promiseSearchEvents(["CurrentEngine"]).then(TestRunner.next);
  yield checkCurrentEngine(ENGINE_2X_LOGO, false, true);

  
  let logo1x2xEngine = null;
  yield promiseNewSearchEngine(ENGINE_1X_2X_LOGO, 2).then(engine => {
    logo1x2xEngine = engine;
    TestRunner.next();
  });
  ok(!!logo1x2xEngine.getIconURLBySize(...LOGO_1X_DPI_SIZE),
     "Sanity check: engine should have 1x logo");
  ok(!!logo1x2xEngine.getIconURLBySize(...LOGO_2X_DPI_SIZE),
     "Sanity check: engine should have 2x logo");
  Services.search.currentEngine = logo1x2xEngine;
  yield promiseSearchEvents(["CurrentEngine"]).then(TestRunner.next);
  yield checkCurrentEngine(ENGINE_1X_2X_LOGO, true, true);

  
  yield Promise.all([
    promisePanelShown(panel),
    promiseClick(logoImg()),
  ]).then(TestRunner.next);

  
  
  let noLogoBox = null;
  for (let box of panel.childNodes) {
    if (box.getAttribute("engine") == noLogoEngine.name) {
      noLogoBox = box;
      break;
    }
  }
  ok(noLogoBox, "Search panel should contain the no-logo engine");
  yield Promise.all([
    promiseSearchEvents(["CurrentEngine"]),
    promiseClick(noLogoBox),
  ]).then(TestRunner.next);

  yield checkCurrentEngine(ENGINE_NO_LOGO, false, false);

  
  Services.search.currentEngine = logo1x2xEngine;
  yield promiseSearchEvents(["CurrentEngine"]).then(TestRunner.next);
  yield checkCurrentEngine(ENGINE_1X_2X_LOGO, true, true);

  
  yield Promise.all([
    promisePanelShown(panel),
    promiseClick(logoImg()),
  ]).then(TestRunner.next);

  
  let manageBox = $("manage");
  ok(!!manageBox, "The Manage Engines box should be present in the document");
  yield Promise.all([
    promiseManagerOpen(),
    promiseClick(manageBox),
  ]).then(TestRunner.next);

  
  let suggestionEngine = null;
  yield promiseNewSearchEngine(ENGINE_SUGGESTIONS, 0).then(engine => {
    suggestionEngine = engine;
    TestRunner.next();
  });
  Services.search.currentEngine = suggestionEngine;
  yield promiseSearchEvents(["CurrentEngine"]).then(TestRunner.next);
  yield checkCurrentEngine(ENGINE_SUGGESTIONS, false, false);

  
  gSearch()._suggestionController.remoteTimeout = 5000;

  
  
  
  let input = $("text");
  input.focus();
  EventUtils.synthesizeKey("x", {});
  let suggestionsPromise = promiseSearchEvents(["Suggestions"]);

  
  
  let table = getContentDocument().getElementById("searchSuggestionTable");
  info("Waiting for suggestions table to open");
  let observer = new MutationObserver(() => {
    if (input.getAttribute("aria-expanded") == "true") {
      observer.disconnect();
      ok(!table.hidden, "Search suggestion table unhidden");
      TestRunner.next();
    }
  });
  observer.observe(input, {
    attributes: true,
    attributeFilter: ["aria-expanded"],
  });
  yield undefined;
  yield suggestionsPromise.then(TestRunner.next);

  
  EventUtils.synthesizeKey("a", { accelKey: true });
  EventUtils.synthesizeKey("VK_DELETE", {});
  ok(table.hidden, "Search suggestion table hidden");

  
  Services.search.currentEngine = oldCurrentEngine;
  yield promiseSearchEvents(["CurrentEngine"]).then(TestRunner.next);

  let events = [];
  for (let engine of gNewEngines) {
    Services.search.removeEngine(engine);
    events.push("CurrentState");
  }
  yield promiseSearchEvents(events).then(TestRunner.next);
}

function searchEventListener(event) {
  info("Got search event " + event.detail.type);
  let passed = false;
  let nonempty = gExpectedSearchEventQueue.length > 0;
  ok(nonempty, "Expected search event queue should be nonempty");
  if (nonempty) {
    let { type, deferred } = gExpectedSearchEventQueue.shift();
    is(event.detail.type, type, "Got expected search event " + type);
    if (event.detail.type == type) {
      passed = true;
      
      executeSoon(() => deferred.resolve());
    }
  }
  if (!passed) {
    info("Didn't get expected event, stopping the test");
    getContentWindow().removeEventListener(SERVICE_EVENT_NAME,
                                           searchEventListener);
    
    TestRunner.next = function () {};
    TestRunner.finish();
  }
}

function $(idSuffix) {
  return getContentDocument().getElementById("newtab-search-" + idSuffix);
}

function promiseSearchEvents(events) {
  info("Expecting search events: " + events);
  events = events.map(e => ({ type: e, deferred: Promise.defer() }));
  gExpectedSearchEventQueue.push(...events);
  return Promise.all(events.map(e => e.deferred.promise));
}

function promiseNewSearchEngine(basename, numLogos) {
  info("Waiting for engine to be added: " + basename);

  
  
  let expectedSearchEvents = ["CurrentState", "CurrentState"];
  
  for (let i = 0; i < numLogos; i++) {
    expectedSearchEvents.push("CurrentState");
  }
  let eventPromise = promiseSearchEvents(expectedSearchEvents);

  
  let addDeferred = Promise.defer();
  let url = getRootDirectory(gTestPath) + basename;
  Services.search.addEngine(url, Ci.nsISearchEngine.TYPE_MOZSEARCH, "", false, {
    onSuccess: function (engine) {
      info("Search engine added: " + basename);
      gNewEngines.push(engine);
      addDeferred.resolve(engine);
    },
    onError: function (errCode) {
      ok(false, "addEngine failed with error code " + errCode);
      addDeferred.reject();
    },
  });

  
  
  
  
  let deferred = Promise.defer();
  Promise.all([addDeferred.promise, eventPromise]).then(values => {
    let newEngine = values[0];
    deferred.resolve(newEngine);
  }, () => deferred.reject());
  return deferred.promise;
}

function checkCurrentEngine(basename, has1xLogo, has2xLogo) {
  let engine = Services.search.currentEngine;
  ok(engine.name.contains(basename),
     "Sanity check: current engine: engine.name=" + engine.name +
     " basename=" + basename);

  
  is(gSearch().currentEngineName, engine.name,
     "currentEngineName: " + engine.name);

  
  let logoURI = null;
  if (window.devicePixelRatio == 2) {
    if (has2xLogo) {
      logoURI = engine.getIconURLBySize(...LOGO_2X_DPI_SIZE);
      ok(!!logoURI, "Sanity check: engine should have 2x logo");
    }
  }
  else {
    if (has1xLogo) {
      logoURI = engine.getIconURLBySize(...LOGO_1X_DPI_SIZE);
      ok(!!logoURI, "Sanity check: engine should have 1x logo");
    }
    else if (has2xLogo) {
      logoURI = engine.getIconURLBySize(...LOGO_2X_DPI_SIZE);
      ok(!!logoURI, "Sanity check: engine should have 2x logo");
    }
  }
  let logo = logoImg();
  is(logo.hidden, !logoURI,
     "Logo should be visible iff engine has a logo: " + engine.name);
  if (logoURI) {
    
    
    ok(/^url\("blob:/.test(logo.style.backgroundImage), "Logo URI"); 
  }

  if (logo.hidden) {
    executeSoon(TestRunner.next);
    return;
  }

  
  let panel = searchPanel();
  promisePanelShown(panel).then(() => {
    panel.hidePopup();
    for (let engineBox of panel.childNodes) {
      let engineName = engineBox.getAttribute("engine");
      if (engineName == engine.name) {
        is(engineBox.getAttribute("selected"), "true",
           "Engine box's selected attribute should be true for " +
           "selected engine: " + engineName);
      }
      else {
        ok(!engineBox.hasAttribute("selected"),
           "Engine box's selected attribute should be absent for " +
           "non-selected engine: " + engineName);
      }
    }
    TestRunner.next();
  });
  panel.openPopup(logo);
}

function promisePanelShown(panel) {
  let deferred = Promise.defer();
  info("Waiting for popupshown");
  panel.addEventListener("popupshown", function onEvent() {
    panel.removeEventListener("popupshown", onEvent);
    is(panel.state, "open", "Panel state");
    executeSoon(() => deferred.resolve());
  });
  return deferred.promise;
}

function promiseClick(node) {
  let deferred = Promise.defer();
  let win = getContentWindow();
  SimpleTest.waitForFocus(() => {
    EventUtils.synthesizeMouseAtCenter(node, {}, win);
    deferred.resolve();
  }, win);
  return deferred.promise;
}

function promiseManagerOpen() {
  info("Waiting for the search manager window to open...");
  let deferred = Promise.defer();
  let winWatcher = Cc["@mozilla.org/embedcomp/window-watcher;1"].
                   getService(Ci.nsIWindowWatcher);
  winWatcher.registerNotification(function onWin(subj, topic, data) {
    if (topic == "domwindowopened" && subj instanceof Ci.nsIDOMWindow) {
      subj.addEventListener("load", function onLoad() {
        subj.removeEventListener("load", onLoad);
        if (subj.document.documentURI ==
            "chrome://browser/content/search/engineManager.xul") {
          winWatcher.unregisterNotification(onWin);
          ok(true, "Observed search manager window opened");
          is(subj.opener, gWindow,
             "Search engine manager opener should be the chrome browser " +
             "window containing the newtab page");
          executeSoon(() => {
            subj.close();
            deferred.resolve();
          });
        }
      });
    }
  });
  return deferred.promise;
}

function searchPanel() {
  return $("panel");
}

function logoImg() {
  return $("logo");
}

function gSearch() {
  return getContentWindow().gSearch;
}
