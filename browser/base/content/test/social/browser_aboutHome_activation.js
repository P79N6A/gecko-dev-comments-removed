



let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AboutHomeUtils",
  "resource:///modules/AboutHome.jsm");

let snippet =
'     <script>' +
'       var manifest = {' +
'         "name": "Demo Social Service",' +
'         "origin": "https://example.com",' +
'         "iconURL": "chrome://branding/content/icon16.png",' +
'         "icon32URL": "chrome://branding/content/favicon32.png",' +
'         "icon64URL": "chrome://branding/content/icon64.png",' +
'         "sidebarURL": "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",' +
'         "postActivationURL": "https://example.com/browser/browser/base/content/test/social/social_postActivation.html",' +
'       };' +
'       function activateProvider(node) {' +
'         node.setAttribute("data-service", JSON.stringify(manifest));' +
'         var event = new CustomEvent("ActivateSocialFeature");' +
'         node.dispatchEvent(event);' +
'       }' +
'     </script>' +
'     <div id="activationSnippet" onclick="activateProvider(this)">' +
'     <img src="chrome://branding/content/favicon32.png"></img>' +
'     </div>';


let snippet2 =
'     <script>' +
'       var manifest = {' +
'         "name": "Demo Social Service",' +
'         "origin": "https://example.com",' +
'         "iconURL": "chrome://branding/content/icon16.png",' +
'         "icon32URL": "chrome://branding/content/favicon32.png",' +
'         "icon64URL": "chrome://branding/content/icon64.png",' +
'         "sidebarURL": "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",' +
'         "postActivationURL": "https://example.com/browser/browser/base/content/test/social/social_postActivation.html",' +
'         "oneclick": true' +
'       };' +
'       function activateProvider(node) {' +
'         node.setAttribute("data-service", JSON.stringify(manifest));' +
'         var event = new CustomEvent("ActivateSocialFeature");' +
'         node.dispatchEvent(event);' +
'       }' +
'     </script>' +
'     <div id="activationSnippet" onclick="activateProvider(this)">' +
'     <img src="chrome://branding/content/favicon32.png"></img>' +
'     </div>';

let gTests = [

{
  desc: "Test activation with enable panel",
  setup: function (aSnippetsMap)
  {
    
    aSnippetsMap.set("snippets", snippet);
  },
  run: function (aSnippetsMap)
  {
    let deferred = Promise.defer();
    let doc = gBrowser.selectedTab.linkedBrowser.contentDocument;

    let snippetsElt = doc.getElementById("snippets");
    ok(snippetsElt, "Found snippets element");
    ok(!!doc.getElementById("activationSnippet"),
       "The snippet is present.");

    activateProvider(gBrowser.selectedTab, true, function() {
      ok(SocialSidebar.provider, "provider activated");
      checkSocialUI();
      is(gBrowser.contentDocument.location.href, SocialSidebar.provider.manifest.postActivationURL);
      gBrowser.removeTab(gBrowser.selectedTab);
      SocialService.uninstallProvider(SocialSidebar.provider.origin, function () {
        info("provider uninstalled");
        aSnippetsMap.delete("snippets");
        deferred.resolve(true);
      });
    });
    return deferred.promise;
  }
},

{
  desc: "Test activation bypassing enable panel",
  setup: function (aSnippetsMap)
  {
    
    aSnippetsMap.set("snippets", snippet2);
  },
  run: function (aSnippetsMap)
  {
    let deferred = Promise.defer();
    let doc = gBrowser.selectedTab.linkedBrowser.contentDocument;

    let snippetsElt = doc.getElementById("snippets");
    ok(snippetsElt, "Found snippets element");
    ok(!!doc.getElementById("activationSnippet"),
       "The snippet is present.");

    activateProvider(gBrowser.selectedTab, false, function() {
      ok(SocialSidebar.provider, "provider activated");
      checkSocialUI();
      is(gBrowser.contentDocument.location.href, SocialSidebar.provider.manifest.postActivationURL);
      gBrowser.removeTab(gBrowser.selectedTab);
      SocialService.uninstallProvider(SocialSidebar.provider.origin, function () {
        info("provider uninstalled");
        aSnippetsMap.delete("snippets");
        deferred.resolve(true);
      });
    });
    return deferred.promise;
  }
}
];

function test()
{
  waitForExplicitFinish();
  requestLongerTimeout(2);
  ignoreAllUncaughtExceptions();

  Task.spawn(function () {
    for (let test of gTests) {
      info(test.desc);

      
      let tab = gBrowser.selectedTab = gBrowser.addTab("about:blank");

      
      let snippetsPromise = promiseSetupSnippetsMap(tab, test.setup);

      
      yield promiseTabLoadEvent(tab, "about:home", "AboutHomeLoadSnippetsCompleted");

      
      
      let snippetsMap = yield snippetsPromise;

      info("Running test");
      let testPromise = test.run(snippetsMap);
      yield testPromise;
      info("Cleanup");
      gBrowser.removeCurrentTab();
    }
  }).then(finish, ex => {
    ok(false, "Unexpected Exception: " + ex);
    finish();
  });
}












function promiseTabLoadEvent(aTab, aURL, aEventType="load")
{
  let deferred = Promise.defer();
  info("Wait tab event: " + aEventType);
  aTab.linkedBrowser.addEventListener(aEventType, function load(event) {
    if (event.originalTarget != aTab.linkedBrowser.contentDocument ||
        event.target.location.href == "about:blank") {
      info("skipping spurious load event");
      return;
    }
    aTab.linkedBrowser.removeEventListener(aEventType, load, true);
    info("Tab event received: " + aEventType);
    deferred.resolve();
  }, true, true);
  aTab.linkedBrowser.loadURI(aURL);
  return deferred.promise;
}











function promiseSetupSnippetsMap(aTab, aSetupFn)
{
  let deferred = Promise.defer();
  info("Waiting for snippets map");
  aTab.linkedBrowser.addEventListener("AboutHomeLoadSnippets", function load(event) {
    aTab.linkedBrowser.removeEventListener("AboutHomeLoadSnippets", load, true);

    let cw = aTab.linkedBrowser.contentWindow.wrappedJSObject;
    
    
    cw.ensureSnippetsMapThen(function (aSnippetsMap) {
      aSnippetsMap = Cu.waiveXrays(aSnippetsMap);
      info("Got snippets map: " +
           "{ last-update: " + aSnippetsMap.get("snippets-last-update") +
           ", cached-version: " + aSnippetsMap.get("snippets-cached-version") +
           " }");
      
      aSnippetsMap.set("snippets-last-update", Date.now());
      aSnippetsMap.set("snippets-cached-version", AboutHomeUtils.snippetsVersion);
      
      aSnippetsMap.delete("snippets");
      aSetupFn(aSnippetsMap);
      deferred.resolve(aSnippetsMap);
    });
  }, true, true);
  return deferred.promise;
}


function sendActivationEvent(tab, callback) {
  
  Social.lastEventReceived = 0;
  let doc = tab.linkedBrowser.contentDocument;
  
  if (doc.defaultView.frames[0])
    doc = doc.defaultView.frames[0].document;
  let button = doc.getElementById("activationSnippet");
  EventUtils.synthesizeMouseAtCenter(button, {}, doc.defaultView);
  executeSoon(callback);
}

function activateProvider(tab, expectPanel, aCallback) {
  if (expectPanel) {
    let panel = document.getElementById("servicesInstall-notification");
    PopupNotifications.panel.addEventListener("popupshown", function onpopupshown() {
      PopupNotifications.panel.removeEventListener("popupshown", onpopupshown);
      panel.button.click();
    });
  }
  sendActivationEvent(tab, function() {
    waitForProviderLoad(function() {
      ok(SocialSidebar.provider, "new provider is active");
      ok(SocialSidebar.opened, "sidebar is open");
      checkSocialUI();
      executeSoon(aCallback);
    });
  });
}

function waitForProviderLoad(cb) {
  Services.obs.addObserver(function providerSet(subject, topic, data) {
    Services.obs.removeObserver(providerSet, "social:provider-enabled");
    info("social:provider-enabled observer was notified");
    waitForCondition(function() {
      let sbrowser = document.getElementById("social-sidebar-browser");
      let provider = SocialSidebar.provider;
      let postActivation = provider && gBrowser.contentDocument.location.href == provider.origin + "/browser/browser/base/content/test/social/social_postActivation.html";

      return provider &&
             postActivation &&
             sbrowser.docShellIsActive;
    }, function() {
      
      executeSoon(cb);
    },
    "waitForProviderLoad: provider profile was not set");
  }, "social:provider-enabled", false);
}
