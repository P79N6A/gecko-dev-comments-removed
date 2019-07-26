










const TEST_URI = "https://example.com/browser/browser/base/content/test/test-mixedcontent-securityerrors.html";
let gTestBrowser = null;

function test()
{
  waitForExplicitFinish();
  SpecialPowers.pushPrefEnv({"set": [["security.mixed_content.block_active_content", true],
                            ["security.mixed_content.block_display_content", true]]}, blockMixedContentTest);
}

function blockMixedContentTest()
{
  gBrowser.selectedTab = gBrowser.addTab(TEST_URI);
  let tab = gBrowser.selectedTab;
  gTestBrowser = gBrowser.getBrowserForTab(tab);

  gTestBrowser.addEventListener("load", function onLoad(aEvent) {
    gTestBrowser.removeEventListener(aEvent.type, onLoad, true);
    is(gTestBrowser.docShell.hasMixedDisplayContentBlocked, true, "hasMixedDisplayContentBlocked flag has been set");
    is(gTestBrowser.docShell.hasMixedActiveContentBlocked, true, "hasMixedActiveContentBlocked flag has been set");
    is(gTestBrowser.docShell.hasMixedDisplayContentLoaded, false, "hasMixedDisplayContentLoaded flag has been set");
    is(gTestBrowser.docShell.hasMixedActiveContentLoaded, false, "hasMixedActiveContentLoaded flag has been set");
    overrideMCB();
  }, true);
}

function overrideMCB()
{
  gTestBrowser.addEventListener("load", mixedContentOverrideTest, true);
  var notification = PopupNotifications.getNotification("mixed-content-blocked", gTestBrowser);
  ok(notification, "Mixed Content Doorhanger didn't appear");
  
  notification.secondaryActions[0].callback(mixedContentOverrideTest);
}

function mixedContentOverrideTest()
{
  gTestBrowser.removeEventListener("load", mixedContentOverrideTest, true);

  is(gTestBrowser.docShell.hasMixedDisplayContentLoaded, true, "hasMixedDisplayContentLoaded flag has not been set");
  is(gTestBrowser.docShell.hasMixedActiveContentLoaded, true, "hasMixedActiveContentLoaded flag has not been set");
  is(gTestBrowser.docShell.hasMixedDisplayContentBlocked, false, "second hasMixedDisplayContentBlocked flag has been set");
  is(gTestBrowser.docShell.hasMixedActiveContentBlocked, false, "second hasMixedActiveContentBlocked flag has been set");
  gBrowser.removeCurrentTab();
  finish();
}
