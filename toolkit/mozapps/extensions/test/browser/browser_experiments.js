



let gManagerWindow;
let gCategoryUtilities;
let gInstalledAddons = [];

function test() {
  waitForExplicitFinish();

  open_manager(null, (win) => {
    gManagerWindow = win;
    gCategoryUtilities = new CategoryUtilities(win);

    run_next_test();
  });
}

function end_test() {
  for (let addon of gInstalledAddons) {
    addon.uninstall();
  }

  close_manager(gManagerWindow, finish);
}



add_test(function testInitialState() {
  Assert.ok(gCategoryUtilities.get("experiment", false), "Experiment tab is defined.");

  Assert.ok(!gCategoryUtilities.isTypeVisible("experiment"), "Experiment tab hidden by default.");

  run_next_test();
});

add_test(function testExperimentInfoNotVisible() {
  gCategoryUtilities.openType("extension", () => {
    let el = gManagerWindow.document.getElementsByClassName("experiment-info-container")[0];
    is_element_hidden(el, "Experiment info not visible on other types.");

    run_next_test();
  });
});



add_test(function testActiveExperiment() {
  install_addon("addons/browser_experiment1.xpi", (addon) => {
    gInstalledAddons.push(addon);

    
    
    Assert.equal(addon.isActive, false, "Add-on is not active.");

    Assert.ok(gCategoryUtilities.isTypeVisible("experiment"), "Experiment tab visible.");

    gCategoryUtilities.openType("experiment", (win) => {
      let el = gManagerWindow.document.getElementsByClassName("experiment-info-container")[0];
      is_element_visible(el, "Experiment info is visible on experiment tab.");

      run_next_test();
    });
  });
});

add_test(function testExperimentLearnMore() {
  
  Services.prefs.setCharPref("toolkit.telemetry.infoURL",
                             "http://mochi.test:8888/server.js");

  gCategoryUtilities.openType("experiment", (win) => {
    let btn = gManagerWindow.document.getElementById("experiments-learn-more");

    if (!gUseInContentUI) {
      is_element_hidden(btn, "Learn more button hidden if not using in-content UI.");
      Services.prefs.clearUserPref("toolkit.telemetry.infoURL");

      run_next_test();
      return;
    } else {
      is_element_visible(btn, "Learn more button visible.");
    }

    window.addEventListener("DOMContentLoaded", function onLoad(event) {
      info("Telemetry privacy policy window opened.");
      window.removeEventListener("DOMContentLoaded", onLoad, false);

      let browser = gBrowser.selectedTab.linkedBrowser;
      let expected = Services.prefs.getCharPref("toolkit.telemetry.infoURL");
      Assert.equal(browser.currentURI.spec, expected, "New tab should have loaded privacy policy.");
      browser.contentWindow.close();

      Services.prefs.clearUserPref("toolkit.telemetry.infoURL");

      run_next_test();
    }, false);

    info("Opening telemetry privacy policy.");
    EventUtils.synthesizeMouseAtCenter(btn, {}, gManagerWindow);
  });
});

add_test(function testOpenPreferences() {
  gCategoryUtilities.openType("experiment", (win) => {
    let btn = gManagerWindow.document.getElementById("experiments-change-telemetry");
    if (!gUseInContentUI) {
      is_element_hidden(btn, "Change telemetry button not enabled in out of window UI.");
      info("Skipping preferences open test because not using in-content UI.");
      run_next_test();
      return;
    }

    is_element_visible(btn, "Change telemetry button visible in in-content UI.");

    Services.obs.addObserver(function observer(prefWin, topic, data) {
      Services.obs.removeObserver(observer, "advanced-pane-loaded");

      info("Advanced preference pane opened.");

      
      let el = prefWin.document.getElementById("dataChoicesPanel");
      is_element_visible(el);

      prefWin.close();
      info("Closed preferences pane.");

      run_next_test();
    }, "advanced-pane-loaded", false);

    info("Loading preferences pane.");
    EventUtils.synthesizeMouseAtCenter(btn, {}, gManagerWindow);
  });
});
