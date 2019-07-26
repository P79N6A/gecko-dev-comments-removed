



let prefName = "social.enabled",
    gFinishCB;

function test() {
  waitForExplicitFinish();

  
  let tab = gBrowser.selectedTab = gBrowser.addTab("https://example.com", {skipAnimation: true});
  tab.linkedBrowser.addEventListener("load", function tabLoad(event) {
    tab.linkedBrowser.removeEventListener("load", tabLoad, true);
    executeSoon(tabLoaded);
  }, true);

  registerCleanupFunction(function() {
    Services.prefs.clearUserPref(prefName);
    gBrowser.removeTab(tab);
  });
}

function tabLoaded() {
  ok(Social, "Social module loaded");

  let manifest = { 
    name: "provider 1",
    origin: "https://example.com",
    sidebarURL: "https://example.com/browser/browser/base/content/test/social/social_sidebar.html",
    workerURL: "https://example.com/browser/browser/base/content/test/social/social_worker.js",
    iconURL: "https://example.com/browser/browser/base/content/test/moz.png"
  };
  runSocialTestWithProvider(manifest, function (finishcb) {
    gFinishCB = finishcb;
    testInitial();
  });
}

function testInitial(finishcb) {
  ok(Social.provider, "Social provider is active");
  ok(Social.provider.enabled, "Social provider is enabled");
  let port = Social.provider.getWorkerPort();
  ok(port, "Social provider has a port to its FrameWorker");
  port.close();

  let markButton = SocialMark.button;
  ok(markButton, "mark button exists");

  
  
  waitForCondition(function() Social.provider.pageMarkInfo != null, function() {
    is(markButton.hasAttribute("marked"), false, "SocialMark button should not have 'marked' attribute before mark button is clicked");
    
    is(markButton.getAttribute("tooltiptext"), "Mark this page", "check tooltip text is correct");
    
    is(markButton.style.listStyleImage, 'url("https://example.com/browser/browser/base/content/test/social/social_mark_image.png")', "check image url is correct");

    
    SocialMark.togglePageMark(function() {
      is(markButton.hasAttribute("marked"), true, "mark button should have 'marked' attribute after mark button is clicked");
      is(markButton.getAttribute("tooltiptext"), "Unmark this page", "check tooltip text is correct");
      
      is(markButton.style.listStyleImage, 'url("https://example.com/browser/browser/base/content/test/social/social_mark_image.png")', "check image url is correct");
      SocialMark.togglePageMark(function() {
        executeSoon(function() {
          testStillMarkedIn2Tabs();
        });
      });
    });
    markButton.click();
  }, "provider didn't provide page-mark-config");
}

function testStillMarkedIn2Tabs() {
  let toMark = "http://example.com";
  let markUri = Services.io.newURI(toMark, null, null);
  let markButton = SocialMark.button;
  let initialTab = gBrowser.selectedTab;
  if (markButton.hasAttribute("marked")) {
    SocialMark.togglePageMark(testStillMarkedIn2Tabs);
    return;
  }
  is(markButton.hasAttribute("marked"), false, "SocialMark button should not have 'marked' for the initial tab");
  let tab1 = gBrowser.selectedTab = gBrowser.addTab(toMark);
  let tab1b = gBrowser.getBrowserForTab(tab1);

  tab1b.addEventListener("load", function tabLoad(event) {
    tab1b.removeEventListener("load", tabLoad, true);
    let tab2 = gBrowser.selectedTab = gBrowser.addTab(toMark);
    let tab2b = gBrowser.getBrowserForTab(tab2);
    tab2b.addEventListener("load", function tabLoad(event) {
      tab2b.removeEventListener("load", tabLoad, true);
      
      is(markButton.hasAttribute("marked"), false, "SocialMark button should not have 'marked' before we've done anything");
      Social.isURIMarked(markUri, function(marked) {
        ok(!marked, "page is unmarked in annotations");
        markButton.click();
        waitForCondition(function() markButton.hasAttribute("marked"), function() {
          Social.isURIMarked(markUri, function(marked) {
            ok(marked, "page is marked in annotations");
            
            gBrowser.selectedTab = tab1;
            is(markButton.hasAttribute("marked"), true, "SocialMark button should reflect the marked state");
            
            gBrowser.selectedTab = initialTab;
            waitForCondition(function() !markButton.hasAttribute("marked"), function() {
              gBrowser.selectedTab = tab1;
    
              SocialMark.togglePageMark(function() {
                Social.isURIMarked(gBrowser.currentURI, function(marked) {
                  ok(!marked, "page is unmarked in annotations");
                  waitForCondition(function() !markButton.hasAttribute("marked"), function() {
                    is(markButton.hasAttribute("marked"), false, "SocialMark button should reflect the marked state");
                    gBrowser.removeTab(tab1);
                    gBrowser.removeTab(tab2);
                    executeSoon(testStillMarkedAfterReopen);
                  }, "button has been unmarked");
                });
              });
            }, "button has been unmarked");
          });
        }, "button has been marked");
      });

    }, true);
  }, true);
}

function testStillMarkedAfterReopen() {
  let toMark = "http://example.com";
  let markButton = SocialMark.button;

  is(markButton.hasAttribute("marked"), false, "Reopen: SocialMark button should not have 'marked' for the initial tab");
  let tab = gBrowser.selectedTab = gBrowser.addTab(toMark);
  let tabb = gBrowser.getBrowserForTab(tab);
  tabb.addEventListener("load", function tabLoad(event) {
    tabb.removeEventListener("load", tabLoad, true);
    SocialMark.togglePageMark(function() {
      is(markButton.hasAttribute("marked"), true, "SocialMark button should reflect the marked state");
      gBrowser.removeTab(tab);
      
      waitForCondition(function() !markButton.hasAttribute("marked"), function() {
        
        tab = gBrowser.selectedTab = gBrowser.addTab(toMark, {skipAnimation: true});
        tab.linkedBrowser.addEventListener("load", function tabLoad(event) {
          tab.linkedBrowser.removeEventListener("load", tabLoad, true);
          executeSoon(function() {
            is(markButton.hasAttribute("marked"), true, "New tab to previously marked URL should reflect marked state");
            SocialMark.togglePageMark(function() {
              gBrowser.removeTab(tab);
              executeSoon(testOnlyMarkCertainUrlsTabSwitch);
            });
          });
        }, true);
      }, "button is now unmarked");
    });
  }, true);
}

function testOnlyMarkCertainUrlsTabSwitch() {
  let toMark = "http://example.com";
  let notSharable = "about:blank";
  let markButton = SocialMark.button;
  let tab = gBrowser.selectedTab = gBrowser.addTab(toMark);
  let tabb = gBrowser.getBrowserForTab(tab);
  tabb.addEventListener("load", function tabLoad(event) {
    tabb.removeEventListener("load", tabLoad, true);
    ok(!markButton.hidden, "SocialMark button not hidden for http url");
    let tab2 = gBrowser.selectedTab = gBrowser.addTab(notSharable);
    let tabb2 = gBrowser.getBrowserForTab(tab2);
    tabb2.addEventListener("load", function tabLoad(event) {
      tabb2.removeEventListener("load", tabLoad, true);
      ok(markButton.disabled, "SocialMark button disabled for about:blank");
      gBrowser.selectedTab = tab;
      ok(!markButton.disabled, "SocialMark button re-shown when switching back to http: url");
      gBrowser.selectedTab = tab2;
      ok(markButton.disabled, "SocialMark button re-hidden when switching back to about:blank");
      gBrowser.removeTab(tab);
      gBrowser.removeTab(tab2);
      executeSoon(testOnlyMarkCertainUrlsSameTab);
    }, true);
  }, true);
}

function testOnlyMarkCertainUrlsSameTab() {
  let toMark = "http://example.com";
  let notSharable = "about:blank";
  let markButton = SocialMark.button;
  let tab = gBrowser.selectedTab = gBrowser.addTab(toMark);
  let tabb = gBrowser.getBrowserForTab(tab);
  tabb.addEventListener("load", function tabLoad(event) {
    tabb.removeEventListener("load", tabLoad, true);
    ok(!markButton.disabled, "SocialMark button not disabled for http url");
    tabb.addEventListener("load", function tabLoad(event) {
      tabb.removeEventListener("load", tabLoad, true);
      ok(markButton.disabled, "SocialMark button disabled for about:blank");
      tabb.addEventListener("load", function tabLoad(event) {
        tabb.removeEventListener("load", tabLoad, true);
        ok(!markButton.disabled, "SocialMark button re-enabled http url");
        gBrowser.removeTab(tab);
        executeSoon(testDisable);
      }, true);
      tabb.loadURI(toMark);
    }, true);
    tabb.loadURI(notSharable);
  }, true);
}

function testDisable() {
  let markButton = SocialMark.button;
  Services.prefs.setBoolPref(prefName, false);
  is(markButton.hidden, true, "SocialMark button should be hidden when pref is disabled");
  gFinishCB();
}
