



const SERVER_URL = "http://example.com/browser/toolkit/crashreporter/test/browser/crashreport.sjs";
const PLUGIN_PAGE = getRootDirectory(gTestPath) + "plugin_big.html";
const PLUGIN_SMALL_PAGE = getRootDirectory(gTestPath) + "plugin_small.html";












function convertPropertyBag(aBag) {
  let result = {};
  let enumerator = aBag.enumerator;
  while(enumerator.hasMoreElements()) {
    let { name, value } = enumerator.getNext().QueryInterface(Ci.nsIProperty);
    if (value instanceof Ci.nsIPropertyBag) {
      value = convertPropertyBag(value);
    }
    result[name] = value;
  }
  return result;
}

function promisePopupNotificationShown(notificationID) {
  return new Promise((resolve) => {
    waitForNotificationShown(notificationID, resolve);
  });
}

add_task(function* setup() {
  setTestPluginEnabledState(Ci.nsIPluginTag.STATE_CLICKTOPLAY);

  
  
  
  
  
  let env = Cc["@mozilla.org/process/environment;1"].
            getService(Components.interfaces.nsIEnvironment);
  let noReport = env.get("MOZ_CRASHREPORTER_NO_REPORT");
  let serverURL = env.get("MOZ_CRASHREPORTER_URL");
  env.set("MOZ_CRASHREPORTER_NO_REPORT", "");
  env.set("MOZ_CRASHREPORTER_URL", SERVER_URL);

  registerCleanupFunction(function cleanUp() {
    env.set("MOZ_CRASHREPORTER_NO_REPORT", noReport);
    env.set("MOZ_CRASHREPORTER_URL", serverURL);
  });
});





add_task(function*() {
  yield BrowserTestUtils.withNewTab({
    gBrowser,
    url: PLUGIN_PAGE,
  }, function* (browser) {
    let activated = yield ContentTask.spawn(browser, null, function*() {
      let plugin = content.document.getElementById("test");
      return plugin.QueryInterface(Ci.nsIObjectLoadingContent).activated;
    });
    ok(!activated, "Plugin should not be activated");

    
    let popupNotification = PopupNotifications.getNotification("click-to-play-plugins",
                                                               browser);
    ok(popupNotification, "Should have a click-to-play notification");

    yield promisePopupNotificationShown(popupNotification);

    
    PopupNotifications.panel.firstChild._primaryButton.click();

    
    
    let crashReportChecker = (subject, data) => {
      return (data == "success");
    };
    let crashReportPromise = TestUtils.topicObserved("crash-report-status",
                                                     crashReportChecker);

    yield ContentTask.spawn(browser, null, function*() {
      let plugin = content.document.getElementById("test");
      plugin.QueryInterface(Ci.nsIObjectLoadingContent);

      yield ContentTaskUtils.waitForCondition(() => {
        return plugin.activated;
      }, "Waited too long for plugin to activate.");

      try {
        plugin.crash();
      } catch(e) {}

      let doc = plugin.ownerDocument;

      let getUI = (anonid) => {
        return doc.getAnonymousElementByAttribute(plugin, "anonid", anonid);
      };

      
      
      let statusDiv;

      yield ContentTaskUtils.waitForCondition(() => {
        statusDiv = getUI("submitStatus");
        return statusDiv.getAttribute("status") == "please";
      }, "Waited too long for plugin to show crash report UI");

      
      let style = content.getComputedStyle(getUI("pleaseSubmit"));
      if (style.display != "block") {
        return Promise.reject(`Submission UI visibility is not correct. ` +
                              `Expected block style, got ${style.display}.`);
      }

      
      
      getUI("submitComment").value = "a test comment";
      let optIn = getUI("submitURLOptIn");
      if (!optIn.checked) {
        return Promise.reject("URL opt-in should default to true.");
      }

      
      optIn.click();
      getUI("submitButton").click();

      
      
      yield ContentTaskUtils.waitForCondition(() => {
        return statusDiv.getAttribute("status") == "success";
      }, "Timed out waiting for plugin binding to be in success state");
    });

    let [subject, data] = yield crashReportPromise;

    ok(subject instanceof Ci.nsIPropertyBag,
       "The crash report subject should be an nsIPropertyBag.");

    let crashData = convertPropertyBag(subject);
    ok(crashData.serverCrashID, "Should have a serverCrashID set.");

    
    let file = Cc["@mozilla.org/file/local;1"]
                 .createInstance(Ci.nsILocalFile);
    file.initWithPath(Services.crashmanager._submittedDumpsDir);
    file.append(crashData.serverCrashID + ".txt");
    ok(file.exists(), "Submitted report file should exist");
    file.remove(false);

    ok(crashData.extra, "Extra data should exist");
    is(crashData.extra.PluginUserComment, "a test comment",
       "Comment in extra data should match comment in textbox");

    is(crashData.extra.PluginContentURL, undefined,
       "URL should be absent from extra data when opt-in not checked");
  });
});





add_task(function*() {
  yield BrowserTestUtils.withNewTab({
    gBrowser,
    url: PLUGIN_SMALL_PAGE,
  }, function* (browser) {
    let activated = yield ContentTask.spawn(browser, null, function*() {
      let plugin = content.document.getElementById("test");
      return plugin.QueryInterface(Ci.nsIObjectLoadingContent).activated;
    });
    ok(!activated, "Plugin should not be activated");

    
    let popupNotification = PopupNotifications.getNotification("click-to-play-plugins",
                                                               browser);
    ok(popupNotification, "Should have a click-to-play notification");

    yield promisePopupNotificationShown(popupNotification);

    
    PopupNotifications.panel.firstChild._primaryButton.click();

    
    
    let crashReportChecker = (subject, data) => {
      return (data == "success");
    };
    let crashReportPromise = TestUtils.topicObserved("crash-report-status",
                                                     crashReportChecker);

    yield ContentTask.spawn(browser, null, function*() {
      let plugin = content.document.getElementById("test");
      plugin.QueryInterface(Ci.nsIObjectLoadingContent);

      yield ContentTaskUtils.waitForCondition(() => {
        return plugin.activated;
      }, "Waited too long for plugin to activate.");

      try {
        plugin.crash();
      } catch(e) {}
    });

    
    let notification = yield waitForNotificationBar("plugin-crashed", browser);

    
    let buttons = notification.querySelectorAll(".notification-button");
    is(buttons.length, 2, "Should have two buttons.");

    
    let submitButton = buttons[1];
    submitButton.click();

    let [subject, data] = yield crashReportPromise;

    ok(subject instanceof Ci.nsIPropertyBag,
       "The crash report subject should be an nsIPropertyBag.");

    let crashData = convertPropertyBag(subject);
    ok(crashData.serverCrashID, "Should have a serverCrashID set.");

    
    let file = Cc["@mozilla.org/file/local;1"]
                 .createInstance(Ci.nsILocalFile);
    file.initWithPath(Services.crashmanager._submittedDumpsDir);
    file.append(crashData.serverCrashID + ".txt");
    ok(file.exists(), "Submitted report file should exist");
    file.remove(false);

    is(crashData.extra.PluginContentURL, undefined,
       "URL should be absent from extra data when opt-in not checked");
  });
});