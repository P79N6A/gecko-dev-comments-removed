



function test() {

  let local = {};

  Components.utils.import("resource://gre/modules/CloudSync.jsm", local);
  Components.utils.import("resource:///modules/sessionstore/TabState.jsm", local);

  let cloudSync = local.CloudSync();
  let opentabs = [];

  waitForExplicitFinish();

  let testURL = "chrome://mochitests/content/browser/services/cloudsync/tests/mochitest/other_window.html";
  let expected = [
    testURL,
    testURL+"?x=1",
    testURL+"?x=%20a",
    
  ];

  let nevents = 0;
  function handleTabChangeEvent () {
    cloudSync.tabs.removeEventListener("change", handleTabChangeEvent);
    ++ nevents;
  }

  function getLocalTabs() {
    cloudSync.tabs.getLocalTabs().then(
      function (tabs) {
        for (let tab of tabs) {
          ok(expected.indexOf(tab.url) >= 0, "found an expected tab");
        }

        is(tabs.length, expected.length, "found the right number of tabs");

        opentabs.forEach(function (tab) {
          gBrowser.removeTab(tab);
        });

        is(nevents, 1, "expected number of change events");

        finish();
      }
    )
  }

  cloudSync.tabs.addEventListener("change", handleTabChangeEvent);

  let nflushed = 0;
  expected.forEach(function(url) {
    let tab = gBrowser.addTab(url);

    function flush() {
      tab.linkedBrowser.removeEventListener("load", flush);
      local.TabState.flush(tab.linkedBrowser);
      ++ nflushed;

      if (nflushed == expected.length) {
        getLocalTabs();
      }
    }

    tab.linkedBrowser.addEventListener("load", flush, true);

    opentabs.push(tab);
  });

}