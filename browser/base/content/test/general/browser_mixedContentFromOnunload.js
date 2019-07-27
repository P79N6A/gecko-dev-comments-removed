









const gHttpTestRoot1 = "http://example.com/browser/browser/base/content/test/general/";
const gHttpsTestRoot1 = "https://test1.example.com/browser/browser/base/content/test/general/";
const gHttpTestRoot2 = "http://example.net/browser/browser/base/content/test/general/";
const gHttpsTestRoot2 = "https://test2.example.com/browser/browser/base/content/test/general/";

let gTestBrowser = null;

function SecStateTestsCompleted() {
  gBrowser.removeCurrentTab();
  window.focus();
  finish();
}

function test() {
  waitForExplicitFinish();
  SpecialPowers.pushPrefEnv({"set": [["security.mixed_content.block_active_content", true],
                            ["security.mixed_content.block_display_content", false]]}, SecStateTests);
}

function SecStateTests() {
  gBrowser.selectedTab = gBrowser.addTab();
  gTestBrowser = gBrowser.selectedBrowser;

  whenLoaded(gTestBrowser, SecStateTest1A);
  let url = gHttpTestRoot1 + "file_mixedContentFromOnunload.html";
  gTestBrowser.contentWindow.location = url;
}



function SecStateTest1A() {
  whenLoaded(gTestBrowser, SecStateTest1B);
  let url = gHttpsTestRoot1 + "file_mixedContentFromOnunload_test1.html";
  gTestBrowser.contentWindow.location = url;
}

function SecStateTest1B() {
  
  
  isSecurityState("secure");

  whenLoaded(gTestBrowser, SecStateTest2A);

  
  let url = gHttpTestRoot2 + "file_mixedContentFromOnunload.html";
  gTestBrowser.contentWindow.location = url;
}



function SecStateTest2A() {
  whenLoaded(gTestBrowser, SecStateTest2B);
  let url = gHttpsTestRoot2 + "file_mixedContentFromOnunload_test2.html";
  gTestBrowser.contentWindow.location = url;
}

function SecStateTest2B() {
  isSecurityState("broken");

  SecStateTestsCompleted();
}


function isSecurityState(expectedState) {
  let ui = gTestBrowser.securityUI;
  if (!ui) {
    ok(false, "No security UI to get the security state");
    return;
  }

  const wpl = Components.interfaces.nsIWebProgressListener;

  
  let isSecure = ui.state & wpl.STATE_IS_SECURE;
  let isBroken = ui.state & wpl.STATE_IS_BROKEN;
  let isInsecure = ui.state & wpl.STATE_IS_INSECURE;

  let actualState;
  if (isSecure && !(isBroken || isInsecure)) {
    actualState = "secure";
  } else if (isBroken && !(isSecure || isInsecure)) {
    actualState = "broken";
  } else if (isInsecure && !(isSecure || isBroken)) {
    actualState = "insecure";
  } else {
    actualState = "unknown";
  }

  is(expectedState, actualState, "Expected state " + expectedState + " and the actual state is " + actualState + ".");
}

function whenLoaded(aElement, aCallback) {
  aElement.addEventListener("load", function onLoad() {
    aElement.removeEventListener("load", onLoad, true);
    executeSoon(aCallback);
  }, true);
}
