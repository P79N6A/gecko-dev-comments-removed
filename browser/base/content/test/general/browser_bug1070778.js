


function* promiseAutoComplete(inputText) {
  gURLBar.focus();
  gURLBar.value = inputText.slice(0, -1);
  EventUtils.synthesizeKey(inputText.slice(-1) , {});
  yield promiseSearchComplete();
}

function is_selected(index) {
  is(gURLBar.popup.richlistbox.selectedIndex, index, `Item ${index + 1} should be selected`);
}

add_task(function*() {
  
  if (!Services.prefs.getBoolPref("browser.urlbar.unifiedcomplete"))
    return;

  let itemIds = [];
  registerCleanupFunction(() => {
    itemIds.forEach(PlacesUtils.bookmarks.removeItem);
  });

  let itemId =
    PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                         NetUtil.newURI("http://example.com/?q=%s"),
                                         PlacesUtils.bookmarks.DEFAULT_INDEX,
                                         "test");
  PlacesUtils.bookmarks.setKeywordForBookmark(itemId, "keyword");
  itemIds.push(itemId);

  
  
  itemId =
    PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                         NetUtil.newURI("http://example.com/keyword"),
                                         PlacesUtils.bookmarks.DEFAULT_INDEX,
                                         "keyword abc");
  itemIds.push(itemId);

  yield promiseAutoComplete("keyword a");

  
  EventUtils.synthesizeKey("VK_DOWN", {});
  is_selected(0);
  
  EventUtils.synthesizeKey("VK_DOWN", {});
  is_selected(1);
  
  EventUtils.synthesizeKey("VK_UP", {});
  is_selected(0);

  EventUtils.synthesizeKey("b", {});
  yield promiseSearchComplete();

  is(gURLBar.value, "keyword ab", "urlbar should have expected input");

  let result = gURLBar.popup.richlistbox.firstChild;
  isnot(result, null, "Should have first item");
  let uri = NetUtil.newURI(result.getAttribute("url"));
  is(uri.spec, makeActionURI("keyword", {url: "http://example.com/?q=ab", input: "keyword ab"}).spec, "Expect correct url");

  EventUtils.synthesizeKey("VK_ESCAPE", {});
  yield promisePopupHidden(gURLBar.popup);
});
