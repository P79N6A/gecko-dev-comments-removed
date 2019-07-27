


function* check_title(inputText, expectedTitle) {
  gURLBar.focus();
  gURLBar.value = inputText.slice(0, -1);
  EventUtils.synthesizeKey(inputText.slice(-1) , {});
  yield promiseSearchComplete();

  ok(gURLBar.popup.richlistbox.children.length > 1, "Should get at least 2 results");
  let result = gURLBar.popup.richlistbox.children[1];
  is(result._title.textContent, expectedTitle, "Result title should be as expected");
}

add_task(function*() {
  
  if (!Services.prefs.getBoolPref("browser.urlbar.unifiedcomplete")) {
    todo(false, "Stop supporting old autocomplete components.");
    return;
  }

  let uri = NetUtil.newURI("http://bug1060642.example.com/beards/are/pretty/great");
  yield PlacesTestUtils.addVisits([{uri: uri, title: ""}]);

  yield check_title("bug1060642", "bug1060642.example.com");

  gURLBar.popup.hidePopup();
  yield promisePopupHidden(gURLBar.popup);
});
