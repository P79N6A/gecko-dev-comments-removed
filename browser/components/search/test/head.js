






function isSubObjectOf(expectedObj, actualObj, name) {
  for (let prop in expectedObj) {
    if (typeof expectedObj[prop] == 'function')
      continue;
    if (expectedObj[prop] instanceof Object) {
      is(actualObj[prop].length, expectedObj[prop].length, name + "[" + prop + "]");
      isSubObjectOf(expectedObj[prop], actualObj[prop], name + "[" + prop + "]");
    } else {
      is(actualObj[prop], expectedObj[prop], name + "[" + prop + "]");
    }
  }
}

function waitForPopupShown(aPopupId, aCallback) {
  let popup = document.getElementById(aPopupId);
  function onPopupShown() {
    popup.removeEventListener("popupshown", onPopupShown);
    SimpleTest.executeSoon(aCallback);
  }
  popup.addEventListener("popupshown", onPopupShown);
}

function waitForBrowserContextMenu(aCallback) {
  waitForPopupShown(gBrowser.selectedBrowser.contextMenu, aCallback);
}
