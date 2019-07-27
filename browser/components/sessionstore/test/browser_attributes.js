


function test() {
  TestRunner.run();
}







const PREF = "browser.sessionstore.restore_on_demand";

function runTests() {
  Services.prefs.setBoolPref(PREF, true)
  registerCleanupFunction(() => Services.prefs.clearUserPref(PREF));

  
  let tab = gBrowser.addTab("about:robots");
  yield whenBrowserLoaded(tab.linkedBrowser);

  
  ok(tab.hasAttribute("image"), "tab.image exists");

  
  ss.persistTabAttribute("image");
  let {attributes} = JSON.parse(ss.getTabState(tab));
  ok(!("image" in attributes), "'image' attribute not saved");
  ok(!("custom" in attributes), "'custom' attribute not saved");

  
  tab.setAttribute("custom", "foobar");
  ss.persistTabAttribute("custom");

  ({attributes} = JSON.parse(ss.getTabState(tab)));
  is(attributes.custom, "foobar", "'custom' attribute is correct");

  
  let state = {
    entries: [{url: "about:mozilla"}],
    attributes: {custom: "foobaz", image: gBrowser.getIcon(tab)}
  };

  
  whenTabRestoring(tab);
  yield ss.setTabState(tab, JSON.stringify(state));

  ok(tab.hasAttribute("pending"), "tab is pending");
  is(gBrowser.getIcon(tab), state.attributes.image, "tab has correct icon");

  
  gBrowser.selectedTab = tab;
  yield whenTabRestored(tab);

  
  ({attributes} = JSON.parse(ss.getTabState(tab)));
  ok(!("image" in attributes), "'image' attribute not saved");
  ok(!("pending" in attributes), "'pending' attribute not saved");
  is(attributes.custom, "foobaz", "'custom' attribute is correct");

  
  gBrowser.removeTab(tab);
}

function whenTabRestoring(tab) {
  tab.addEventListener("SSTabRestoring", function onRestoring() {
    tab.removeEventListener("SSTabRestoring", onRestoring);
    executeSoon(next);
  });
}
