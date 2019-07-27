


add_task(function* test_no_keyword() {
  Assert.deepEqual({ href: null, postData: null },
                   (yield PlacesUtils.promiseHrefAndPostDataForKeyword("test")),
                   "Keyword 'test' should not exist");
});

add_task(function* test_add_remove() {
  let item1 = yield PlacesUtils.bookmarks.insert({ type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                   parentGuid: PlacesUtils.bookmarks.unfiledGuid,
                                                   url: "http://example1.com/",
                                                   keyword: "test" });
  Assert.deepEqual({ href: item1.url.href, postData: null },
                   (yield PlacesUtils.promiseHrefAndPostDataForKeyword("test")),
                   "Keyword 'test' should point to " + item1.url.href);

  
  
  let item2 = yield PlacesUtils.bookmarks.insert({ type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                   parentGuid: PlacesUtils.bookmarks.unfiledGuid,
                                                   url: "http://example2.com/",
                                                   keyword: "test" });
  Assert.deepEqual({ href: item2.url.href, postData: null },
                   (yield PlacesUtils.promiseHrefAndPostDataForKeyword("test")),
                   "Keyword 'test' should point to " + item2.url.href);

  
  yield PlacesUtils.bookmarks.remove(item2);
  Assert.deepEqual({ href: item1.url.href, postData: null },
                   (yield PlacesUtils.promiseHrefAndPostDataForKeyword("test")),
                   "Keyword 'test' should point to " + item1.url.href);

  
  yield PlacesUtils.bookmarks.remove(item1);
  Assert.deepEqual({ href: null, postData: null },
                   (yield PlacesUtils.promiseHrefAndPostDataForKeyword("test")),
                   "Keyword 'test' should not exist");
});

add_task(function* test_change_url() {
  let item = yield PlacesUtils.bookmarks.insert({ type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                  parentGuid: PlacesUtils.bookmarks.unfiledGuid,
                                                  url: "http://example.com/",
                                                  keyword: "test" });
  Assert.deepEqual({ href: item.url.href, postData: null },
                   (yield PlacesUtils.promiseHrefAndPostDataForKeyword("test")),
                   "Keyword 'test' should point to " + item.url.href);

  
  let updatedItem = yield PlacesUtils.bookmarks.update({ guid: item.guid,
                                                         url: "http://example2.com" });
  Assert.deepEqual({ href: updatedItem.url.href, postData: null },
                   (yield PlacesUtils.promiseHrefAndPostDataForKeyword("test")),
                   "Keyword 'test' should point to " + updatedItem.url.href);
  yield PlacesUtils.bookmarks.remove(updatedItem);
});

add_task(function* test_change_keyword() {
  let item = yield PlacesUtils.bookmarks.insert({ type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                  parentGuid: PlacesUtils.bookmarks.unfiledGuid,
                                                  url: "http://example.com/",
                                                  keyword: "test" });
  Assert.deepEqual({ href: item.url.href, postData: null },
                   (yield PlacesUtils.promiseHrefAndPostDataForKeyword("test")),
                   "Keyword 'test' should point to " + item.url.href);

  
  let updatedItem = yield PlacesUtils.bookmarks.update({ guid: item.guid,
                                                         keyword: "test2" });
  Assert.deepEqual({ href: null, postData: null },
                   (yield PlacesUtils.promiseHrefAndPostDataForKeyword("test")),
                   "Keyword 'test' should not exist");
  Assert.deepEqual({ href: updatedItem.url.href, postData: null },
                   (yield PlacesUtils.promiseHrefAndPostDataForKeyword("test2")),
                   "Keyword 'test' should point to " + updatedItem.url.href);

  
  updatedItem = yield PlacesUtils.bookmarks.update({ guid: item.guid,
                                                     keyword: "" });
  Assert.deepEqual({ href: null, postData: null },
                   (yield PlacesUtils.promiseHrefAndPostDataForKeyword("test")),
                   "Keyword 'test' should not exist");
  Assert.deepEqual({ href: null, postData: null },
                   (yield PlacesUtils.promiseHrefAndPostDataForKeyword("test2")),
                   "Keyword 'test' should not exist");
  yield PlacesUtils.bookmarks.remove(updatedItem);
});

add_task(function* test_postData() {
  let item1 = yield PlacesUtils.bookmarks.insert({ type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                   parentGuid: PlacesUtils.bookmarks.unfiledGuid,
                                                   url: "http://example1.com/",
                                                   keyword: "test" });
  let itemId1 = yield PlacesUtils.promiseItemId(item1.guid);
  PlacesUtils.setPostDataForBookmark(itemId1, "testData");
  Assert.deepEqual({ href: item1.url.href, postData: "testData" },
                   (yield PlacesUtils.promiseHrefAndPostDataForKeyword("test")),
                   "Keyword 'test' should point to " + item1.url.href);

  
  let item2 = yield PlacesUtils.bookmarks.insert({ type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                   parentGuid: PlacesUtils.bookmarks.unfiledGuid,
                                                   url: "http://example2.com/",
                                                   keyword: "test" });
  Assert.deepEqual({ href: item2.url.href, postData: null },
                   (yield PlacesUtils.promiseHrefAndPostDataForKeyword("test")),
                   "Keyword 'test' should point to " + item2.url.href);

  yield PlacesUtils.bookmarks.remove(item1);
  yield PlacesUtils.bookmarks.remove(item2);
});

function run_test() {
  run_next_test();
}
