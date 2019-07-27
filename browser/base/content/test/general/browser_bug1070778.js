


function is_selected(index) {
  is(gURLBar.popup.richlistbox.selectedIndex, index, `Item ${index + 1} should be selected`);
}

add_task(function*() {
  
  if (!Services.prefs.getBoolPref("browser.urlbar.unifiedcomplete")) {
    todo(false, "Stop supporting old autocomplete components.");
    return;
  }

  registerCleanupFunction(() => {
    PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);
  });

  let itemId =
    PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                         NetUtil.newURI("http://example.com/?q=%s"),
                                         PlacesUtils.bookmarks.DEFAULT_INDEX,
                                         "test");
  PlacesUtils.bookmarks.setKeywordForBookmark(itemId, "keyword");

  
  
  itemId =
    PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                         NetUtil.newURI("http://example.com/keyword"),
                                         PlacesUtils.bookmarks.DEFAULT_INDEX,
                                         "keyword abc");

  let tab = gBrowser.selectedTab = gBrowser.addTab("about:mozilla", {animate: false});
  yield promiseTabLoaded(tab);
  yield promiseAutocompleteResultPopup("keyword a");

  
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
  gBrowser.removeTab(tab);
});
