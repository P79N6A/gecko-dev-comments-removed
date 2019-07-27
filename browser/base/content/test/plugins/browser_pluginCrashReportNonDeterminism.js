



















const CRASH_URL = "http://example.com/browser/browser/base/content/test/plugins/plugin_crashCommentAndURL.html";
const CRASHED_MESSAGE = "BrowserPlugins:NPAPIPluginProcessCrashed";




























function preparePlugin(browser, pluginFallbackState) {
  return ContentTask.spawn(browser, pluginFallbackState, function* (pluginFallbackState) {
    let plugin = content.document.getElementById("plugin");
    plugin.QueryInterface(Ci.nsIObjectLoadingContent);
    
    
    
    let statusDiv;
    yield ContentTaskUtils.waitForCondition(() => {
      statusDiv = plugin.ownerDocument
                        .getAnonymousElementByAttribute(plugin, "anonid",
                                                        "submitStatus");
      return statusDiv && statusDiv.getAttribute("status") == "please";
    }, "Timed out waiting for plugin to be in crash report state");

    
    statusDiv.removeAttribute("status");
    
    
    Object.defineProperty(plugin, "pluginFallbackType", {
      get: function() {
        return pluginFallbackState;
      }
    });
    return plugin.runID;
  }).then((runID) => {
    browser.messageManager.sendAsyncMessage("BrowserPlugins:Test:ClearCrashData");
    return runID;
  });
}

add_task(function* setup() {
  
  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_ENABLED);

  
  
  let crashObserver = (subject, topic, data) => {
    if (topic != "plugin-crashed") {
      return;
    }

    let propBag = subject.QueryInterface(Ci.nsIPropertyBag2);
    let minidumpID = propBag.getPropertyAsAString("pluginDumpID");

    let minidumpDir = Services.dirsvc.get("ProfD", Ci.nsIFile);
    minidumpDir.append("minidumps");

    let pluginDumpFile = minidumpDir.clone();
    pluginDumpFile.append(minidumpID + ".dmp");

    let extraFile = minidumpDir.clone();
    extraFile.append(minidumpID + ".extra");

    ok(pluginDumpFile.exists(), "Found minidump");
    ok(extraFile.exists(), "Found extra file");

    pluginDumpFile.remove(false);
    extraFile.remove(false);
  };

  Services.obs.addObserver(crashObserver, "plugin-crashed");
  
  Services.prefs.setBoolPref("plugins.testmode", true);
  registerCleanupFunction(() => {
    Services.prefs.clearUserPref("plugins.testmode");
    Services.obs.removeObserver(crashObserver, "plugin-crashed");
  });
});




add_task(function* testChromeHearsPluginCrashFirst() {
  
  
  let win = yield BrowserTestUtils.openNewBrowserWindow({remote: true});
  let browser = win.gBrowser.selectedBrowser;

  browser.loadURI(CRASH_URL);
  yield BrowserTestUtils.browserLoaded(browser);

  
  
  
  
  let runID = yield preparePlugin(browser,
                                  Ci.nsIObjectLoadingContent.PLUGIN_ACTIVE);

  
  
  let mm = browser.messageManager;
  mm.sendAsyncMessage(CRASHED_MESSAGE,
                      { pluginName: "", runID, state: "please" });

  let [gotExpected, msg] = yield ContentTask.spawn(browser, {}, function* () {
    
    
    
    let plugin = content.document.getElementById("plugin");
    plugin.QueryInterface(Ci.nsIObjectLoadingContent);
    let statusDiv = plugin.ownerDocument
                          .getAnonymousElementByAttribute(plugin, "anonid",
                                                          "submitStatus");

    if (statusDiv.getAttribute("status") == "please") {
      return [false, "Did not expect plugin to be in crash report mode yet."];
    }

    
    
    
    Object.defineProperty(plugin, "pluginFallbackType", {
      get: function() {
        return Ci.nsIObjectLoadingContent.PLUGIN_CRASHED;
      },
    });

    let event = new content.PluginCrashedEvent("PluginCrashed", {
      pluginName: "",
      pluginDumpID: "",
      browserDumpID: "",
      submittedCrashReport: false,
      bubbles: true,
      cancelable: true,
    });

    plugin.dispatchEvent(event);
    return [statusDiv.getAttribute("status") == "please",
            "Should have been showing crash report UI"];
  });

  ok(gotExpected, msg);
  yield BrowserTestUtils.closeWindow(win);
});




add_task(function* testContentHearsCrashFirst() {
  
  
  let win = yield BrowserTestUtils.openNewBrowserWindow({remote: true});
  let browser = win.gBrowser.selectedBrowser;

  browser.loadURI(CRASH_URL);
  yield BrowserTestUtils.browserLoaded(browser);

  
  
  
  let runID = yield preparePlugin(browser,
                                  Ci.nsIObjectLoadingContent.PLUGIN_CRASHED);

  let [gotExpected, msg] = yield ContentTask.spawn(browser, null, function* () {
    
    
    
    let plugin = content.document.getElementById("plugin");
    plugin.QueryInterface(Ci.nsIObjectLoadingContent);
    let statusDiv = plugin.ownerDocument
                          .getAnonymousElementByAttribute(plugin, "anonid",
                                                          "submitStatus");

    if (statusDiv.getAttribute("status") == "please") {
      return [false, "Did not expect plugin to be in crash report mode yet."];
    }

    let event = new content.PluginCrashedEvent("PluginCrashed", {
      pluginName: "",
      pluginDumpID: "",
      browserDumpID: "",
      submittedCrashReport: false,
      bubbles: true,
      cancelable: true,
    });

    plugin.dispatchEvent(event);

    return [statusDiv.getAttribute("status") != "please",
            "Should not yet be showing crash report UI"];
  });

  ok(gotExpected, msg);

  
  
  let mm = browser.messageManager;
  mm.sendAsyncMessage(CRASHED_MESSAGE,
                      { pluginName: "", runID, state: "please"});

  [gotExpected, msg] = yield ContentTask.spawn(browser, null, function* () {
    
    
    
    let plugin = content.document.getElementById("plugin");
    plugin.QueryInterface(Ci.nsIObjectLoadingContent);
    let statusDiv = plugin.ownerDocument
                          .getAnonymousElementByAttribute(plugin, "anonid",
                                                          "submitStatus");

    return [statusDiv.getAttribute("status") == "please",
            "Should have been showing crash report UI"];
  });

  ok(gotExpected, msg);

  yield BrowserTestUtils.closeWindow(win);
});
