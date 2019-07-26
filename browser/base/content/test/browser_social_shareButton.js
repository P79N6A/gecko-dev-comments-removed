



let prefName = "social.enabled",
    gFinishCB;

function test() {
  waitForExplicitFinish();

  
  let tab = gBrowser.selectedTab = gBrowser.addTab("about:", {skipAnimation: true});
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
    sidebarURL: "https://example.com/browser/browser/base/content/test/social_sidebar.html",
    workerURL: "https://example.com/browser/browser/base/content/test/social_worker.js",
    iconURL: "chrome://branding/content/icon48.png"
  };
  runSocialTestWithProvider(manifest, function (finishcb) {
    gFinishCB = finishcb;
    testInitial();
  });
}

function testInitial(finishcb) {
  ok(Social.provider, "Social provider is active");
  ok(Social.provider.enabled, "Social provider is enabled");
  ok(Social.provider.port, "Social provider has a port to its FrameWorker");

  let {shareButton, sharePopup} = SocialShareButton;
  ok(shareButton, "share button exists");
  ok(sharePopup, "share popup exists");

  let okButton = document.getElementById("editSharePopupOkButton");
  let undoButton = document.getElementById("editSharePopupUndoButton");
  let shareStatusLabel = document.getElementById("share-button-status");

  
  
  waitForCondition(function() Social.provider.profile && SocialShareButton.promptImages != null, function() {
    is(shareButton.hasAttribute("shared"), false, "Share button should not have 'shared' attribute before share button is clicked");
    
    let profile = Social.provider.profile;
    let portrait = document.getElementById("socialUserPortrait").getAttribute("src");
    is(profile.portrait, portrait, "portrait is set");
    let displayName = document.getElementById("socialUserDisplayName");
    is(displayName.label, profile.displayName, "display name is set");
    ok(!document.getElementById("editSharePopupHeader").hidden, "user profile is visible");
  
    
    is(shareButton.getAttribute("tooltiptext"), "Share this page", "check tooltip text is correct");
    is(shareStatusLabel.getAttribute("value"), "", "check status label text is blank");
    
    is(shareButton.style.backgroundImage, 'url("https://example.com/browser/browser/base/content/test/social_share_image.png")', "check image url is correct");

    
    shareButton.addEventListener("click", function listener() {
      shareButton.removeEventListener("click", listener);
      is(shareButton.hasAttribute("shared"), true, "Share button should have 'shared' attribute after share button is clicked");
      is(shareButton.getAttribute("tooltiptext"), "Unshare this page", "check tooltip text is correct");
      is(shareStatusLabel.getAttribute("value"), "This page has been shared", "check status label text is correct");
      
      is(shareButton.style.backgroundImage, 'url("https://example.com/browser/browser/base/content/test/social_share_image.png")', "check image url is correct");
      executeSoon(testSecondClick.bind(window, testPopupOKButton));
    });
    EventUtils.synthesizeMouseAtCenter(shareButton, {});
  }, "provider didn't provide user-recommend-prompt response");
}

function testSecondClick(nextTest) {
  let {shareButton, sharePopup} = SocialShareButton;
  sharePopup.addEventListener("popupshown", function listener() {
    sharePopup.removeEventListener("popupshown", listener);
    ok(true, "popup was shown after second click");
    executeSoon(nextTest);
  });
  EventUtils.synthesizeMouseAtCenter(shareButton, {});
}

function testPopupOKButton() {
  let {shareButton, sharePopup} = SocialShareButton;
  let okButton = document.getElementById("editSharePopupOkButton");
  sharePopup.addEventListener("popuphidden", function listener() {
    sharePopup.removeEventListener("popuphidden", listener);
    is(shareButton.hasAttribute("shared"), true, "Share button should still have 'shared' attribute after OK button is clicked");
    executeSoon(testSecondClick.bind(window, testPopupUndoButton));
  });
  EventUtils.synthesizeMouseAtCenter(okButton, {});
}

function testPopupUndoButton() {
  let {shareButton, sharePopup} = SocialShareButton;
  let undoButton = document.getElementById("editSharePopupUndoButton");
  sharePopup.addEventListener("popuphidden", function listener() {
    sharePopup.removeEventListener("popuphidden", listener);
    is(shareButton.hasAttribute("shared"), false, "Share button should not have 'shared' attribute after Undo button is clicked");
    executeSoon(testShortcut);
  });
  EventUtils.synthesizeMouseAtCenter(undoButton, {});
}

function testShortcut() {
  let keyTarget = window;
  keyTarget.addEventListener("keyup", function listener() {
    keyTarget.removeEventListener("keyup", listener);
    executeSoon(checkShortcutWorked.bind(window, keyTarget));
  });
  EventUtils.synthesizeKey("l", {accelKey: true, shiftKey: true}, keyTarget);
}

function checkShortcutWorked(keyTarget) {
  let {sharePopup, shareButton} = SocialShareButton;
  is(shareButton.hasAttribute("shared"), true, "Share button should be in the 'shared' state after keyboard shortcut is used");

  
  sharePopup.addEventListener("popupshown", function listener() {
    sharePopup.removeEventListener("popupshown", listener);
    ok(true, "popup was shown after second use of keyboard shortcut");
    executeSoon(checkOKButton);
  });
  EventUtils.synthesizeKey("l", {accelKey: true, shiftKey: true}, keyTarget);
}

function checkOKButton() {
  let okButton = document.getElementById("editSharePopupOkButton");
  let undoButton = document.getElementById("editSharePopupUndoButton");
  is(document.activeElement, okButton, "ok button should be focused by default");

  
  
  if (navigator.platform.contains("Mac")) {
    executeSoon(testCloseBySpace);
    return;
  }

  let displayName = document.getElementById("socialUserDisplayName");

  
  if (navigator.platform.contains("Linux")) {
    checkNextInTabOrder(displayName, function () {
      checkNextInTabOrder(undoButton, function () {
        checkNextInTabOrder(okButton, testCloseBySpace);
      });
    });
  } else {
    checkNextInTabOrder(undoButton, function () {
      checkNextInTabOrder(displayName, function () {
        checkNextInTabOrder(okButton, testCloseBySpace);
      });
    });
  }
}

function checkNextInTabOrder(element, next) {
  function listener() {
    element.removeEventListener("focus", listener);
    is(document.activeElement, element, element.id + " should be next in tab order");
    executeSoon(next);
  }
  element.addEventListener("focus", listener);
  
  registerCleanupFunction(function () {
    element.removeEventListener("focus", listener);
  });
  EventUtils.synthesizeKey("VK_TAB", {});
}

function testCloseBySpace() {
  let sharePopup = SocialShareButton.sharePopup;
  is(document.activeElement.id, "editSharePopupOkButton", "testCloseBySpace, the ok button should be focused");
  sharePopup.addEventListener("popuphidden", function listener() {
    sharePopup.removeEventListener("popuphidden", listener);
    ok(true, "space closed the share popup");
    executeSoon(testStillSharedIn2Tabs);
  });
  EventUtils.synthesizeKey("VK_SPACE", {});
}

function testStillSharedIn2Tabs() {
  let toShare = "http://example.com";
  let {shareButton} = SocialShareButton;
  let initialTab = gBrowser.selectedTab;
  if (shareButton.hasAttribute("shared")) {
    SocialShareButton.unsharePage();
  }
  is(shareButton.hasAttribute("shared"), false, "Share button should not have 'shared' for the initial tab");
  let tab1 = gBrowser.selectedTab = gBrowser.addTab(toShare);
  let tab1b = gBrowser.getBrowserForTab(tab1);

  tab1b.addEventListener("load", function tabLoad(event) {
    tab1b.removeEventListener("load", tabLoad, true);
    let tab2 = gBrowser.selectedTab = gBrowser.addTab(toShare);
    let tab2b = gBrowser.getBrowserForTab(tab2);
    tab2b.addEventListener("load", function tabLoad(event) {
      tab2b.removeEventListener("load", tabLoad, true);
      
      is(shareButton.hasAttribute("shared"), false, "Share button should not have 'shared' before we've done anything");
      EventUtils.synthesizeMouseAtCenter(shareButton, {});
      is(shareButton.hasAttribute("shared"), true, "Share button should reflect the share");
      
      gBrowser.selectedTab = tab1;
      is(shareButton.hasAttribute("shared"), true, "Share button should reflect the share");
      
      gBrowser.selectedTab = initialTab;
      is(shareButton.hasAttribute("shared"), false, "Initial tab should not reflect shared");

      gBrowser.selectedTab = tab1;
      SocialShareButton.unsharePage();
      gBrowser.removeTab(tab1);
      gBrowser.removeTab(tab2);
      executeSoon(testStillSharedAfterReopen);
    }, true);
  }, true);
}

function testStillSharedAfterReopen() {
  let toShare = "http://example.com";
  let {shareButton} = SocialShareButton;

  is(shareButton.hasAttribute("shared"), false, "Reopen: Share button should not have 'shared' for the initial tab");
  let tab = gBrowser.selectedTab = gBrowser.addTab(toShare);
  let tabb = gBrowser.getBrowserForTab(tab);
  tabb.addEventListener("load", function tabLoad(event) {
    tabb.removeEventListener("load", tabLoad, true);
    SocialShareButton.sharePage();
    is(shareButton.hasAttribute("shared"), true, "Share button should reflect the share");
    gBrowser.removeTab(tab);
    
    is(shareButton.hasAttribute("shared"), false, "Initial tab should be selected and be unshared.");
    
    tab = gBrowser.selectedTab = gBrowser.addTab(toShare, {skipAnimation: true});
    tab.linkedBrowser.addEventListener("load", function tabLoad(event) {
      tab.linkedBrowser.removeEventListener("load", tabLoad, true);
      is(shareButton.hasAttribute("shared"), true, "New tab to previously shared URL should reflect shared");
      SocialShareButton.unsharePage();
      gBrowser.removeTab(tab);
      executeSoon(testDisable);
    }, true);
  }, true);
}

function testDisable() {
  let shareButton = SocialShareButton.shareButton;
  Services.prefs.setBoolPref(prefName, false);
  is(shareButton.hidden, true, "Share button should be hidden when pref is disabled");
  gFinishCB();
}
