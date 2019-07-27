





Cu.import("resource://gre/modules/Task.jsm");

const ENGINE_NO_LOGO = {
  name: "searchEngineNoLogo.xml",
  numLogos: 0,
};

const ENGINE_FAVICON = {
  name: "searchEngineFavicon.xml",
  logoPrefix1x: "data:image/png;base64,AAABAAIAICAAAAEAIACoEAAAJgAAABAQAAABACAAaAQAAM4QAAAoAAAAIAAAAEAAAAABACAAAAAAAAAQAAATCwAAEwsA",
  numLogos: 1,
};
ENGINE_FAVICON.logoPrefix2x = ENGINE_FAVICON.logoPrefix1x;

const ENGINE_1X_LOGO = {
  name: "searchEngine1xLogo.xml",
  logoPrefix1x: "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEEAAAAaCAIAAABn3KYmAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH3gkTADEw",
  numLogos: 1,
};
ENGINE_1X_LOGO.logoPrefix2x = ENGINE_1X_LOGO.logoPrefix1x;

const ENGINE_2X_LOGO = {
  name: "searchEngine2xLogo.xml",
  logoPrefix2x: "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAIIAAAA0CAIAAADJ8nfCAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH3gkTADMU",
  numLogos: 1,
};
ENGINE_2X_LOGO.logoPrefix1x = ENGINE_2X_LOGO.logoPrefix2x;

const ENGINE_1X_2X_LOGO = {
  name: "searchEngine1x2xLogo.xml",
  logoPrefix1x: "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEEAAAAaCAIAAABn3KYmAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH3gkTADIG",
  logoPrefix2x: "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAIIAAAA0CAIAAADJ8nfCAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH3gkTADMo",
  numLogos: 2,
};

const ENGINE_SUGGESTIONS = {
  name: "searchSuggestionEngine.xml",
  numLogos: 0,
};

const SERVICE_EVENT_NAME = "ContentSearchService";

const LOGO_1X_DPI_SIZE = [65, 26];
const LOGO_2X_DPI_SIZE = [130, 52];









var gExpectedSearchEventQueue = [];

var gNewEngines = [];

function runTests() {
  runTaskifiedTests().then(TestRunner.next, TestRunner.next);
  yield;
}

let runTaskifiedTests = Task.async(function* () {
  let oldCurrentEngine = Services.search.currentEngine;

  yield addNewTabPageTabPromise();

  
  
  info("Adding search event listener");
  getContentWindow().addEventListener(SERVICE_EVENT_NAME, searchEventListener);

  let panel = searchPanel();
  is(panel.state, "closed", "Search panel should be closed initially");

  
  
  panel.setAttribute("animate", "false");

  
  let noLogoEngine = yield promiseNewSearchEngine(ENGINE_NO_LOGO);
  Services.search.currentEngine = noLogoEngine;
  yield promiseSearchEvents(["CurrentEngine"]);
  yield checkCurrentEngine(ENGINE_NO_LOGO);

  
  let faviconEngine = yield promiseNewSearchEngine(ENGINE_FAVICON);
  Services.search.currentEngine = faviconEngine;
  yield promiseSearchEvents(["CurrentEngine"]);
  yield checkCurrentEngine(ENGINE_FAVICON);

  
  let logo1xEngine = yield promiseNewSearchEngine(ENGINE_1X_LOGO);
  Services.search.currentEngine = logo1xEngine;
  yield promiseSearchEvents(["CurrentEngine"]);
  yield checkCurrentEngine(ENGINE_1X_LOGO);

  
  let logo2xEngine = yield promiseNewSearchEngine(ENGINE_2X_LOGO);
  Services.search.currentEngine = logo2xEngine;
  yield promiseSearchEvents(["CurrentEngine"]);
  yield checkCurrentEngine(ENGINE_2X_LOGO);

  
  let logo1x2xEngine = yield promiseNewSearchEngine(ENGINE_1X_2X_LOGO);
  Services.search.currentEngine = logo1x2xEngine;
  yield promiseSearchEvents(["CurrentEngine"]);
  yield checkCurrentEngine(ENGINE_1X_2X_LOGO);

  
  yield Promise.all([
    promisePanelShown(panel),
    promiseClick(logoImg()),
  ]);

  let manageBox = $("manage");
  ok(!!manageBox, "The Manage Engines box should be present in the document");
  is(panel.childNodes.length, 1, "Search panel should only contain the Manage Engines entry");
  is(panel.childNodes[0], manageBox, "Search panel should contain the Manage Engines entry");

  panel.hidePopup();

  
  let suggestionEngine = yield promiseNewSearchEngine(ENGINE_SUGGESTIONS);
  Services.search.currentEngine = suggestionEngine;
  yield promiseSearchEvents(["CurrentEngine"]);
  yield checkCurrentEngine(ENGINE_SUGGESTIONS);

  
  gSearch()._suggestionController.remoteTimeout = 5000;

  
  
  
  let input = $("text");
  input.focus();
  EventUtils.synthesizeKey("x", {});
  let suggestionsPromise = promiseSearchEvents(["Suggestions"]);

  
  
  let suggestionsUnhiddenDefer = Promise.defer();
  let table = getContentDocument().getElementById("searchSuggestionTable");
  info("Waiting for suggestions table to open");
  let observer = new MutationObserver(() => {
    if (input.getAttribute("aria-expanded") == "true") {
      observer.disconnect();
      ok(!table.hidden, "Search suggestion table unhidden");
      suggestionsUnhiddenDefer.resolve();
    }
  });
  observer.observe(input, {
    attributes: true,
    attributeFilter: ["aria-expanded"],
  });
  yield suggestionsUnhiddenDefer.promise;
  yield suggestionsPromise;

  
  EventUtils.synthesizeKey("a", { accelKey: true });
  EventUtils.synthesizeKey("VK_DELETE", {});
  ok(table.hidden, "Search suggestion table hidden");

  
  CustomizableUI.removeWidgetFromArea("search-container");
  
  let btn = getContentDocument().getElementById("newtab-customize-button");
  yield promiseClick(btn);

  isnot(input, getContentDocument().activeElement, "Search input should not be focused");
  
  EventUtils.synthesizeKey("k", { accelKey: true });
  yield promiseSearchEvents(["FocusInput"]);
  is(input, getContentDocument().activeElement, "Search input should be focused");
  
  CustomizableUI.reset();

  
  let searchBar = gWindow.document.getElementById("searchbar");
  EventUtils.synthesizeKey("k", { accelKey: true });
  is(searchBar.textbox.inputField, gWindow.document.activeElement, "Toolbar's search bar should be focused");

  
  
  yield addNewTabPageTabPromise();
  
  CustomizableUI.removeWidgetFromArea("search-container");
  NewTabUtils.allPages.enabled = false;
  EventUtils.synthesizeKey("k", { accelKey: true });
  let waitEvent = "AboutHomeLoadSnippetsCompleted";
  yield promiseTabLoadEvent(gWindow.gBrowser.selectedTab, "about:home", waitEvent);

  is(getContentDocument().documentURI.toLowerCase(), "about:home", "New tab's uri should be about:home");
  let searchInput = getContentDocument().getElementById("searchText");
  is(searchInput, getContentDocument().activeElement, "Search input must be the selected element");

  NewTabUtils.allPages.enabled = true;
  CustomizableUI.reset();
  gBrowser.removeCurrentTab();

  
  Services.search.currentEngine = oldCurrentEngine;
  yield promiseSearchEvents(["CurrentEngine"]);

  let events = [];
  for (let engine of gNewEngines) {
    Services.search.removeEngine(engine);
    events.push("CurrentState");
  }
  yield promiseSearchEvents(events);
});

function searchEventListener(event) {
  info("Got search event " + event.detail.type);
  let nonempty = gExpectedSearchEventQueue.length > 0;
  ok(nonempty, "Expected search event queue should be nonempty");
  if (nonempty) {
    let { type, deferred } = gExpectedSearchEventQueue.shift();
    is(event.detail.type, type, "Got expected search event " + type);
    if (event.detail.type == type) {
      deferred.resolve();
    } else {
      deferred.reject();
    }
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

function promiseNewSearchEngine({name: basename, numLogos}) {
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

  return Promise.all([addDeferred.promise, eventPromise]).then(([newEngine, _]) => {
    return newEngine;
  });
}

function objectURLToBlob(url) {
  return new Promise(function (resolve, reject) {
    let xhr = new XMLHttpRequest();
    xhr.open("get", url, true);
    xhr.responseType = "blob";
    xhr.overrideMimeType("image/png");
    xhr.onload = function(e) {
      if (this.status == 200) {
        return resolve(this.response);
      }
      reject("Failed to get logo, xhr returned status: " + this.status);
    };
    xhr.onerror = reject;
    xhr.send();
  });
}

function blobToBase64(blob) {
  return new Promise(function (resolve, reject) {
    var reader = new FileReader();
    reader.onload = function() {
      resolve(reader.result);
    }
    reader.onerror = reject;
    reader.readAsDataURL(blob);
  });
}

let checkCurrentEngine = Task.async(function* ({name: basename, logoPrefix1x, logoPrefix2x}) {
  let engine = Services.search.currentEngine;
  ok(engine.name.includes(basename),
     "Sanity check: current engine: engine.name=" + engine.name +
     " basename=" + basename);

  
  is(gSearch().currentEngineName, engine.name,
     "currentEngineName: " + engine.name);
});

function promisePanelShown(panel) {
  let deferred = Promise.defer();
  info("Waiting for popupshown");
  panel.addEventListener("popupshown", function onEvent() {
    panel.removeEventListener("popupshown", onEvent);
    is(panel.state, "open", "Panel state");
    deferred.resolve();
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

function searchPanel() {
  return $("panel");
}

function logoImg() {
  return $("logo");
}

function gSearch() {
  return getContentWindow().gSearch;
}















function promiseTabLoadEvent(tab, url, eventType="load") {
  let deferred = Promise.defer();
  info("Wait tab event: " + eventType);

  function handle(event) {
    if (event.originalTarget != tab.linkedBrowser.contentDocument ||
        event.target.location.href == "about:blank" ||
        (url && event.target.location.href != url)) {
      info("Skipping spurious '" + eventType + "'' event" +
           " for " + event.target.location.href);
      return;
    }
    clearTimeout(timeout);
    tab.linkedBrowser.removeEventListener(eventType, handle, true);
    info("Tab event received: " + eventType);
    deferred.resolve(event);
  }

  let timeout = setTimeout(() => {
    tab.linkedBrowser.removeEventListener(eventType, handle, true);
    deferred.reject(new Error("Timed out while waiting for a '" + eventType + "'' event"));
  }, 20000);

  tab.linkedBrowser.addEventListener(eventType, handle, true, true);
  if (url)
    tab.linkedBrowser.loadURI(url);
  return deferred.promise;
}
