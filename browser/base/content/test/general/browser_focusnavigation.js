



function doTab(expectedTarget)
{
  let focusEventPromise = BrowserTestUtils.waitForEvent(expectedTarget, "focus", true);
  EventUtils.synthesizeKey("VK_TAB", { });
  return focusEventPromise;
}

add_task(function* () {
  let testPage = "data:text/html,<html id='html1'><body id='body1'>Some Text<input id='input1'></body></html>";

  yield BrowserTestUtils.openNewForegroundTab(gBrowser, testPage);

  gURLBar.focus();

  
  EventUtils.synthesizeKey("VK_TAB", { });
  is(document.activeElement, document.getElementById("searchbar").textbox.inputField, "searchbar focused");

  
  
  yield ContentTask.spawn(gBrowser.selectedBrowser, { }, function* (arg) {
    let selection = content.document.getSelection();
    selection.modify("move", "right", "character");
  });

  
  yield doTab(gBrowser.selectedBrowser);

  is(document.activeElement, gBrowser.selectedBrowser, "browser focused");
  let focusedId = yield ContentTask.spawn(gBrowser.selectedBrowser, { }, function* (arg) {
    return content.document.activeElement.id;
  });
  is(focusedId, "html1", "html focused");

  
  EventUtils.synthesizeKey("VK_TAB", { });
  is(document.activeElement, gBrowser.selectedBrowser, "browser still focused");
  focusedId = yield ContentTask.spawn(gBrowser.selectedBrowser, { }, function* (arg) {
    return content.document.activeElement.id;
  });
  is(focusedId, "input1", "input focused");

  
  yield doTab(document);
  is(document.activeElement.localName, "tab", "tab focused");

  gBrowser.removeCurrentTab();
});

