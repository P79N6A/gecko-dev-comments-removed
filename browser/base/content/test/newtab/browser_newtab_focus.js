





function runTests() {
  
  Services.prefs.setIntPref("accessibility.tabfocus", 7);

  
  
  
  let FOCUS_COUNT = 28;

  
  yield setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("");

  yield addNewTabPageTab();
  gURLBar.focus();

  
  yield countFocus(FOCUS_COUNT);

  
  NewTabUtils.allPages.enabled = false;
  yield countFocus(1);

  Services.prefs.clearUserPref("accessibility.tabfocus");
}




function countFocus(aExpectedCount) {
  let focusCount = 0;
  let contentDoc = getContentDocument();

  window.addEventListener("focus", function onFocus() {
    let focusedElement = document.commandDispatcher.focusedElement;
    if (focusedElement && focusedElement.classList.contains("urlbar-input")) {
      window.removeEventListener("focus", onFocus, true);
      is(focusCount, aExpectedCount, "Validate focus count in the new tab page.");
      executeSoon(TestRunner.next);
    } else {
      if (focusedElement && focusedElement.ownerDocument == contentDoc &&
          focusedElement instanceof HTMLElement) {
        focusCount++;
      }
      document.commandDispatcher.advanceFocus();
    }
  }, true);

  document.commandDispatcher.advanceFocus();
}
