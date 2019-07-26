



function test() {
  waitForExplicitFinish();
  ok(true, "Starting up");

  gBrowser.selectedBrowser.focus();
  gURLBar.addEventListener("focus", function onFocus() {
    gURLBar.removeEventListener("focus", onFocus);
    ok(true, "Invoked onfocus handler");
    EventUtils.synthesizeKey("VK_RETURN", { shiftKey: true });

    
    SimpleTest.executeSoon(function() {
      ok(true, "Evaluated without crashing");
      finish();
    });
  });
  gURLBar.inputField.value = "javascript: var foo = '11111111'; ";
  gURLBar.focus();
}
