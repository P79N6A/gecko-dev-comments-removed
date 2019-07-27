


function* check_a11y_label(inputText, expectedLabel) {
  gURLBar.focus();
  gURLBar.value = inputText.slice(0, -1);
  EventUtils.synthesizeKey(inputText.slice(-1) , {});
  yield promiseSearchComplete();

  ok(gURLBar.popup.richlistbox.children.length > 1, "Should get at least 2 results");
  let result = gURLBar.popup.richlistbox.children[1];
  is(result.getAttribute("type"), "action switchtab", "Expect right type attribute");
  is(result.label, expectedLabel, "Result a11y label should be as expected");
}

add_task(function*() {
  
  if (!Services.prefs.getBoolPref("browser.urlbar.unifiedcomplete"))
    return;

  let tab = gBrowser.addTab("about:about");
  yield promiseTabLoaded(tab);

  let actionURL = makeActionURI("switchtab", {url: "about:about"}).spec;
  yield check_a11y_label("% about", "about:about " + actionURL + " Tab");

  yield promisePopupHidden(gURLBar.popup);
  gBrowser.removeTab(tab);
});
