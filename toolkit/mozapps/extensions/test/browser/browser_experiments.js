



let gManagerWindow;
let gCategoryUtilities;
let gInstalledAddons = [];
let gContext = this;

add_task(function* initializeState() {
  gManagerWindow = yield open_manager();
  gCategoryUtilities = new CategoryUtilities(gManagerWindow);

  
  
  
  
  if ("@mozilla.org/browser/experiments-service;1" in Components.classes) {
    Components.utils.import("resource:///modules/experiments/Experiments.jsm", gContext);

    
    
    
    let instance = gContext.Experiments.instance();
    yield instance.uninit();
  }
});



add_task(function* testInitialState() {
  Assert.ok(gCategoryUtilities.get("experiment", false), "Experiment tab is defined.");

  Assert.ok(!gCategoryUtilities.isTypeVisible("experiment"), "Experiment tab hidden by default.");
});

add_task(function* testExperimentInfoNotVisible() {
  yield gCategoryUtilities.openType("extension");
  let el = gManagerWindow.document.getElementsByClassName("experiment-info-container")[0];
  is_element_hidden(el, "Experiment info not visible on other types.");
});



add_task(function* testActiveExperiment() {
  let addon = yield install_addon("addons/browser_experiment1.xpi");
  gInstalledAddons.push(addon);

  Assert.ok(addon.userDisabled, "Add-on is disabled upon initial install.");
  Assert.equal(addon.isActive, false, "Add-on is not active.");

  Assert.ok(gCategoryUtilities.isTypeVisible("experiment"), "Experiment tab visible.");

  yield gCategoryUtilities.openType("experiment");
  let el = gManagerWindow.document.getElementsByClassName("experiment-info-container")[0];
  is_element_visible(el, "Experiment info is visible on experiment tab.");
});

add_task(function* testExperimentLearnMore() {
  
  Services.prefs.setCharPref("toolkit.telemetry.infoURL",
                             "http://mochi.test:8888/server.js");

  yield gCategoryUtilities.openType("experiment");
  let btn = gManagerWindow.document.getElementById("experiments-learn-more");

  if (!gUseInContentUI) {
    is_element_hidden(btn, "Learn more button hidden if not using in-content UI.");
    Services.prefs.clearUserPref("toolkit.telemetry.infoURL");

    return;
  }

  is_element_visible(btn, "Learn more button visible.");

  let deferred = Promise.defer();
  window.addEventListener("DOMContentLoaded", function onLoad(event) {
    info("Telemetry privacy policy window opened.");
    window.removeEventListener("DOMContentLoaded", onLoad, false);

    let browser = gBrowser.selectedTab.linkedBrowser;
    let expected = Services.prefs.getCharPref("toolkit.telemetry.infoURL");
    Assert.equal(browser.currentURI.spec, expected, "New tab should have loaded privacy policy.");
    browser.contentWindow.close();

    Services.prefs.clearUserPref("toolkit.telemetry.infoURL");

    deferred.resolve();
  }, false);

  info("Opening telemetry privacy policy.");
  EventUtils.synthesizeMouseAtCenter(btn, {}, gManagerWindow);

  yield deferred.promise;
});

add_task(function* testOpenPreferences() {
  yield gCategoryUtilities.openType("experiment");
  let btn = gManagerWindow.document.getElementById("experiments-change-telemetry");
  if (!gUseInContentUI) {
    is_element_hidden(btn, "Change telemetry button not enabled in out of window UI.");
    info("Skipping preferences open test because not using in-content UI.");
    return;
  }

  is_element_visible(btn, "Change telemetry button visible in in-content UI.");

  let deferred = Promise.defer();
  Services.obs.addObserver(function observer(prefWin, topic, data) {
    Services.obs.removeObserver(observer, "advanced-pane-loaded");

    info("Advanced preference pane opened.");

    
    let el = prefWin.document.getElementById("dataChoicesPanel");
    is_element_visible(el);

    prefWin.close();
    info("Closed preferences pane.");

    deferred.resolve();
  }, "advanced-pane-loaded", false);

  info("Loading preferences pane.");
  EventUtils.synthesizeMouseAtCenter(btn, {}, gManagerWindow);

  yield deferred.promise;
});

add_task(function* testButtonPresence() {
  yield gCategoryUtilities.openType("experiment");
  let item = get_addon_element(gManagerWindow, "test-experiment1@experiments.mozilla.org");
  Assert.ok(item, "Got add-on element.");

  let el = item.ownerDocument.getAnonymousElementByAttribute(item, "anonid", "remove-btn");
  
  is_element_visible(el, "Remove button is visible.");
  
  el = item.ownerDocument.getAnonymousElementByAttribute(item, "anonid", "disable-btn");
  is_element_hidden(el, "Disable button not visible.");
  
  el = item.ownerDocument.getAnonymousElementByAttribute(item, "anonid", "enable-btn");
  is_element_hidden(el, "Enable button not visible.");
});

add_task(function* testCleanup() {
  for (let addon of gInstalledAddons) {
    addon.uninstall();
  }

  yield close_manager(gManagerWindow);

  if ("@mozilla.org/browser/experiments-service;1" in Components.classes) {
    yield gContext.Experiments.instance().init();
  }
});
