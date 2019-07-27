


add_task(function*() {
  
  if (!Services.prefs.getBoolPref("browser.urlbar.unifiedcomplete")) {
    todo(false, "Stop supporting old autocomplete components.");
    return;
  }

  let tab = gBrowser.addTab("about:about");
  yield promiseTabLoaded(tab);

  let actionURL = makeActionURI("switchtab", {url: "about:about"}).spec;
  yield promiseAutocompleteResultPopup("% about");

  ok(gURLBar.popup.richlistbox.children.length > 1, "Should get at least 2 results");
  let result = gURLBar.popup.richlistbox.children[1];
  is(result.getAttribute("type"), "action switchtab", "Expect right type attribute");
  is(result.label, "about:about " + actionURL + " Tab", "Result a11y label should be as expected");

  gURLBar.popup.hidePopup();
  yield promisePopupHidden(gURLBar.popup);
  gBrowser.removeTab(tab);
});
