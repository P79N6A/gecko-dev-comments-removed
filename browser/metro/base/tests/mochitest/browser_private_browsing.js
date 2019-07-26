




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

    let child = Browser.addTab("about:mozilla", false, tab);
    is(child.isPrivate, false, "Child of a non-private tab is not private");

    Browser.closeTab(child, { forceClose: true });
    Browser.closeTab(tab, { forceClose: true });

    tab = Browser.addTab("about:mozilla", false, null, { private: true });
    is(tab.isPrivate, true, "Create a private tab");
    is(tab.chromeTab.getAttribute("private"), "true",
      "private tab has private attribute");

    child = Browser.addTab("about:mozilla", false, tab);
    is(child.isPrivate, true, "Child of a private tab is private");

    Browser.closeTab(child, { forceClose: true });
    Browser.closeTab(tab, { forceClose: true });
  }
});
