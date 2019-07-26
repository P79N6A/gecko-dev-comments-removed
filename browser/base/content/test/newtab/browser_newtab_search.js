





const ENGINE_LOGO = "searchEngineLogo.xml";
const ENGINE_NO_LOGO = "searchEngineNoLogo.xml";

const SERVICE_EVENT_NAME = "ContentSearchService";

const LOGO_LOW_DPI_SIZE = [65, 26];
const LOGO_HIGH_DPI_SIZE = [130, 52];









var gExpectedSearchEventQueue = [];

var gNewEngines = [];

function runTests() {
  let oldCurrentEngine = Services.search.currentEngine;

  yield addNewTabPageTab();
  yield whenSearchInitDone();

  
  
  info("Adding search event listener");
  getContentWindow().addEventListener(SERVICE_EVENT_NAME, searchEventListener);

  let panel = searchPanel();
  is(panel.state, "closed", "Search panel should be closed initially");

  
  
  panel.setAttribute("animate", "false");

  
  let logoEngine = null;
  yield promiseNewSearchEngine(true).then(engine => {
    logoEngine = engine;
    TestRunner.next();
  });
  ok(!!logoEngine.getIconURLBySize(...LOGO_LOW_DPI_SIZE),
     "Sanity check: engine should have 1x logo");
  ok(!!logoEngine.getIconURLBySize(...LOGO_HIGH_DPI_SIZE),
     "Sanity check: engine should have 2x logo");

  let noLogoEngine = null;
  yield promiseNewSearchEngine(false).then(engine => {
    noLogoEngine = engine;
    TestRunner.next();
  });
  ok(!noLogoEngine.getIconURLBySize(...LOGO_LOW_DPI_SIZE),
     "Sanity check: engine should not have 1x logo");
  ok(!noLogoEngine.getIconURLBySize(...LOGO_HIGH_DPI_SIZE),
     "Sanity check: engine should not have 2x logo");

  
  Services.search.currentEngine = logoEngine;
  yield promiseSearchEvents(["CurrentEngine"]).then(TestRunner.next);
  checkCurrentEngine(ENGINE_LOGO);

  
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

  checkCurrentEngine(ENGINE_NO_LOGO);

  
  Services.search.currentEngine = logoEngine;
  yield promiseSearchEvents(["CurrentEngine"]).then(TestRunner.next);
  checkCurrentEngine(ENGINE_LOGO);

  
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

  
  Services.search.currentEngine = oldCurrentEngine;
  yield promiseSearchEvents(["CurrentEngine"]).then(TestRunner.next);

  let events = [];
  for (let engine of gNewEngines) {
    Services.search.removeEngine(engine);
    events.push("State");
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

function promiseNewSearchEngine(withLogo) {
  let basename = withLogo ? ENGINE_LOGO : ENGINE_NO_LOGO;
  info("Waiting for engine to be added: " + basename);

  
  
  let expectedSearchEvents = ["State", "State"];
  if (withLogo) {
    
    expectedSearchEvents.push("State", "State");
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

function checkCurrentEngine(basename) {
  let engine = Services.search.currentEngine;
  ok(engine.name.contains(basename),
     "Sanity check: current engine: engine.name=" + engine.name +
     " basename=" + basename);

  
  is(gSearch().currentEngineName, engine.name,
     "currentEngineName: " + engine.name);

  
  let logoSize = [px * window.devicePixelRatio for (px of LOGO_LOW_DPI_SIZE)];
  let logoURI = engine.getIconURLBySize(...logoSize);
  let logo = logoImg();
  is(logo.hidden, !logoURI,
     "Logo should be visible iff engine has a logo: " + engine.name);
  if (logoURI) {
    is(logo.style.backgroundImage, 'url("' + logoURI + '")', "Logo URI");
  }

  
  let panel = searchPanel();
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
