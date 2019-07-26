




"use strict";

function test() {
  runTests();
}

gTests.push({
  desc: "Private tab sanity check",
  run: function() {
    let tab = Browser.addTab("about:mozilla");
    is(tab.isPrivate, false, "Tabs are not private by default");
    is(tab.chromeTab.hasAttribute("private"), false,
      "non-private tab has no private attribute");
    Browser.closeTab(tab, { forceClose: true });

    tab = Browser.addTab("about:mozilla", false, null, { private: true });
    is(tab.isPrivate, true, "Create a private tab");
    is(tab.chromeTab.getAttribute("private"), "true",
      "private tab has private attribute");
    Browser.closeTab(tab, { forceClose: true });
  }
});
