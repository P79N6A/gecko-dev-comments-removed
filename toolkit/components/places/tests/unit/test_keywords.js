"use strict"

function* check_keyword(aExpectExists, aHref, aKeyword, aPostData = null) {
  
  aKeyword = aKeyword.toUpperCase();

  let entry = yield PlacesUtils.keywords.fetch(aKeyword);

  Assert.deepEqual(entry, yield PlacesUtils.keywords.fetch({ keyword: aKeyword }));

  if (aExpectExists) {
    Assert.ok(!!entry, "A keyword should exist");
    Assert.equal(entry.url.href, aHref);
    Assert.equal(entry.postData, aPostData);
    Assert.deepEqual(entry, yield PlacesUtils.keywords.fetch({ keyword: aKeyword, url: aHref }));
    let entries = [];
    yield PlacesUtils.keywords.fetch({ url: aHref }, e => entries.push(e));
    Assert.ok(entries.some(e => e.url.href == aHref && e.keyword == aKeyword.toLowerCase()));
  } else {
    Assert.ok(!entry || entry.url.href != aHref,
              "The given keyword entry should not exist");
    Assert.equal(null, yield PlacesUtils.keywords.fetch({ keyword: aKeyword, url: aHref }));
  }
}




function* promiseKeyword(keyword, expectedHref) {
  let href = null;
  do {
    yield new Promise(resolve => do_timeout(100, resolve));
    let entry = yield PlacesUtils.keywords.fetch(keyword);
    if (entry)
      href = entry.url.href; 
  } while (href != expectedHref);
}

function* check_no_orphans() {
  let db = yield PlacesUtils.promiseDBConnection();
  let rows = yield db.executeCached(
    `SELECT id FROM moz_keywords k
     WHERE NOT EXISTS (SELECT 1 FROM moz_places WHERE id = k.place_id)
    `);
  Assert.equal(rows.length, 0);
}

function expectBookmarkNotifications() {
  let notifications = [];
  let observer = new Proxy(NavBookmarkObserver, {
    get(target, name) {
      if (name == "check") {
        PlacesUtils.bookmarks.removeObserver(observer);
        return expectedNotifications =>
          Assert.deepEqual(notifications, expectedNotifications);
      }

      if (name.startsWith("onItemChanged")) {
        return (itemId, property) => {
          if (property != "keyword")
            return;
          let args = Array.from(arguments, arg => {
            if (arg && arg instanceof Ci.nsIURI)
              return new URL(arg.spec);
            if (arg && typeof(arg) == "number" && arg >= Date.now() * 1000)
              return new Date(parseInt(arg/1000));
            return arg;
          });
          notifications.push({ name: name, arguments: args });
        }
      }

      if (name in target)
        return target[name];
      return undefined;
    }
  });
  PlacesUtils.bookmarks.addObserver(observer, false);
  return observer;
}

add_task(function* test_invalid_input() {
  Assert.throws(() => PlacesUtils.keywords.fetch(null),
                /Invalid keyword/);
  Assert.throws(() => PlacesUtils.keywords.fetch(5),
                /Invalid keyword/);
  Assert.throws(() => PlacesUtils.keywords.fetch(undefined),
                /Invalid keyword/);
  Assert.throws(() => PlacesUtils.keywords.fetch({ keyword: null }),
                /Invalid keyword/);
  Assert.throws(() => PlacesUtils.keywords.fetch({ keyword: {} }),
                /Invalid keyword/);
  Assert.throws(() => PlacesUtils.keywords.fetch({ keyword: 5 }),
                /Invalid keyword/);
  Assert.throws(() => PlacesUtils.keywords.fetch({}),
                /At least keyword or url must be provided/);
  Assert.throws(() => PlacesUtils.keywords.fetch({ keyword: "test" }, "test"),
                /onResult callback must be a valid function/);
  Assert.throws(() => PlacesUtils.keywords.fetch({ url: "test" }),
                /is not a valid URL/);
  Assert.throws(() => PlacesUtils.keywords.fetch({ url: {} }),
                /is not a valid URL/);
  Assert.throws(() => PlacesUtils.keywords.fetch({ url: null }),
                /is not a valid URL/);
  Assert.throws(() => PlacesUtils.keywords.fetch({ url: "" }),
                /is not a valid URL/);

  Assert.throws(() => PlacesUtils.keywords.insert(null),
                /Input should be a valid object/);
  Assert.throws(() => PlacesUtils.keywords.insert("test"),
                /Input should be a valid object/);
  Assert.throws(() => PlacesUtils.keywords.insert(undefined),
                /Input should be a valid object/);
  Assert.throws(() => PlacesUtils.keywords.insert({ }),
                /Invalid keyword/);
  Assert.throws(() => PlacesUtils.keywords.insert({ keyword: null }),
                /Invalid keyword/);
  Assert.throws(() => PlacesUtils.keywords.insert({ keyword: 5 }),
                /Invalid keyword/);
  Assert.throws(() => PlacesUtils.keywords.insert({ keyword: "" }),
                /Invalid keyword/);
  Assert.throws(() => PlacesUtils.keywords.insert({ keyword: "test", postData: 5 }),
                /Invalid POST data/);
  Assert.throws(() => PlacesUtils.keywords.insert({ keyword: "test", postData: {} }),
                /Invalid POST data/);
  Assert.throws(() => PlacesUtils.keywords.insert({ keyword: "test" }),
                /is not a valid URL/);
  Assert.throws(() => PlacesUtils.keywords.insert({ keyword: "test", url: 5 }),
                /is not a valid URL/);
  Assert.throws(() => PlacesUtils.keywords.insert({ keyword: "test", url: "" }),
                /is not a valid URL/);
  Assert.throws(() => PlacesUtils.keywords.insert({ keyword: "test", url: null }),
                /is not a valid URL/);
  Assert.throws(() => PlacesUtils.keywords.insert({ keyword: "test", url: "mozilla" }),
                /is not a valid URL/);

  Assert.throws(() => PlacesUtils.keywords.remove(null),
                /Invalid keyword/);
  Assert.throws(() => PlacesUtils.keywords.remove(""),
                /Invalid keyword/);
  Assert.throws(() => PlacesUtils.keywords.remove(5),
                /Invalid keyword/);
});

add_task(function* test_addKeyword() {
  yield check_keyword(false, "http://example.com/", "keyword");
  let fc = yield foreign_count("http://example.com/");
  let observer = expectBookmarkNotifications();

  yield PlacesUtils.keywords.insert({ keyword: "keyword", url: "http://example.com/" });
  observer.check([]);

  yield check_keyword(true, "http://example.com/", "keyword");
  Assert.equal((yield foreign_count("http://example.com/")), fc + 1); 

  
  observer = expectBookmarkNotifications();
  yield PlacesUtils.keywords.remove("keyword");
  observer.check([]);

  yield check_keyword(false, "http://example.com/", "keyword");
  Assert.equal((yield foreign_count("http://example.com/")), fc); 

  
  yield PlacesUtils.keywords.insert({ keyword: "keyword", url: new URL("http://example.com/") });
  yield check_keyword(true, "http://example.com/", "keyword");
  yield PlacesUtils.keywords.remove("keyword");
  yield check_keyword(false, "http://example.com/", "keyword");

  yield check_no_orphans();
});

add_task(function* test_addBookmarkAndKeyword() {
  yield check_keyword(false, "http://example.com/", "keyword");
  let fc = yield foreign_count("http://example.com/");
  let bookmark = yield PlacesUtils.bookmarks.insert({ url: "http://example.com/",
                                                      type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                      parentGuid: PlacesUtils.bookmarks.unfiledGuid });

  let observer = expectBookmarkNotifications();
  yield PlacesUtils.keywords.insert({ keyword: "keyword", url: "http://example.com/" });

  observer.check([{ name: "onItemChanged",
                    arguments: [ (yield PlacesUtils.promiseItemId(bookmark.guid)),
                                 "keyword", false, "keyword",
                                 bookmark.lastModified, bookmark.type,
                                 (yield PlacesUtils.promiseItemId(bookmark.parentGuid)),
                                 bookmark.guid, bookmark.parentGuid ] } ]);

  yield check_keyword(true, "http://example.com/", "keyword");
  Assert.equal((yield foreign_count("http://example.com/")), fc + 2); 

  
  observer = expectBookmarkNotifications();
  yield PlacesUtils.keywords.remove("keyword");

  observer.check([{ name: "onItemChanged",
                    arguments: [ (yield PlacesUtils.promiseItemId(bookmark.guid)),
                                 "keyword", false, "",
                                 bookmark.lastModified, bookmark.type,
                                 (yield PlacesUtils.promiseItemId(bookmark.parentGuid)),
                                 bookmark.guid, bookmark.parentGuid ] } ]);

  yield check_keyword(false, "http://example.com/", "keyword");
  Assert.equal((yield foreign_count("http://example.com/")), fc + 1); 

  
  yield PlacesUtils.keywords.insert({ keyword: "keyword", url: "http://example.com/" });

  observer = expectBookmarkNotifications();
  yield PlacesUtils.bookmarks.remove(bookmark.guid);
  
  
  while ((yield foreign_count("http://example.com/")));
  
  
  observer.check([]);

  yield check_keyword(false, "http://example.com/", "keyword");

  yield check_no_orphans();
});

add_task(function* test_addKeywordToURIHavingKeyword() {
  yield check_keyword(false, "http://example.com/", "keyword");
  let fc = yield foreign_count("http://example.com/");

  let observer = expectBookmarkNotifications();
  yield PlacesUtils.keywords.insert({ keyword: "keyword", url: "http://example.com/" });
  observer.check([]);

  yield check_keyword(true, "http://example.com/", "keyword");
  Assert.equal((yield foreign_count("http://example.com/")), fc + 1); 

  yield PlacesUtils.keywords.insert({ keyword: "keyword2", url: "http://example.com/" });

  yield check_keyword(true, "http://example.com/", "keyword");
  yield check_keyword(true, "http://example.com/", "keyword2");
  Assert.equal((yield foreign_count("http://example.com/")), fc + 2); 
  let entries = [];
  let entry = yield PlacesUtils.keywords.fetch({ url: "http://example.com/" }, e => entries.push(e));
  Assert.equal(entries.length, 2);
  Assert.deepEqual(entries[0], entry);

  
  observer = expectBookmarkNotifications();
  yield PlacesUtils.keywords.remove("keyword");
  yield PlacesUtils.keywords.remove("keyword2");
  observer.check([]);

  yield check_keyword(false, "http://example.com/", "keyword");
  yield check_keyword(false, "http://example.com/", "keyword2");
  Assert.equal((yield foreign_count("http://example.com/")), fc); 

  yield check_no_orphans();
});

add_task(function* test_addBookmarkToURIHavingKeyword() {
  yield check_keyword(false, "http://example.com/", "keyword");
  let fc = yield foreign_count("http://example.com/");
  let observer = expectBookmarkNotifications();

  yield PlacesUtils.keywords.insert({ keyword: "keyword", url: "http://example.com/" });
  observer.check([]);

  yield check_keyword(true, "http://example.com/", "keyword");
  Assert.equal((yield foreign_count("http://example.com/")), fc + 1); 

  observer = expectBookmarkNotifications();
  let bookmark = yield PlacesUtils.bookmarks.insert({ url: "http://example.com/",
                                                      type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                      parentGuid: PlacesUtils.bookmarks.unfiledGuid });
  Assert.equal((yield foreign_count("http://example.com/")), fc + 2); 
  observer.check([]);

  observer = expectBookmarkNotifications();
  yield PlacesUtils.bookmarks.remove(bookmark.guid);
  
  
  while ((yield foreign_count("http://example.com/")));
  
  
  observer.check([]);

  yield check_keyword(false, "http://example.com/", "keyword");

  yield check_no_orphans();
});

add_task(function* test_sameKeywordDifferentURL() {
  let fc1 = yield foreign_count("http://example1.com/");
  let bookmark1 = yield PlacesUtils.bookmarks.insert({ url: "http://example1.com/",
                                                       type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                       parentGuid: PlacesUtils.bookmarks.unfiledGuid });
  let fc2 = yield foreign_count("http://example2.com/");
  let bookmark2 = yield PlacesUtils.bookmarks.insert({ url: "http://example2.com/",
                                                       type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                       parentGuid: PlacesUtils.bookmarks.unfiledGuid });
  yield PlacesUtils.keywords.insert({ keyword: "keyword", url: "http://example1.com/" });

  yield check_keyword(true, "http://example1.com/", "keyword");
  Assert.equal((yield foreign_count("http://example1.com/")), fc1 + 2); 
  yield check_keyword(false, "http://example2.com/", "keyword");
  Assert.equal((yield foreign_count("http://example2.com/")), fc2 + 1); 

  
  let observer = expectBookmarkNotifications();
  yield PlacesUtils.keywords.insert({ keyword: "keyword", url: "http://example2.com/" });

  observer.check([{ name: "onItemChanged",
                    arguments: [ (yield PlacesUtils.promiseItemId(bookmark1.guid)),
                                 "keyword", false, "",
                                 bookmark1.lastModified, bookmark1.type,
                                 (yield PlacesUtils.promiseItemId(bookmark1.parentGuid)),
                                 bookmark1.guid, bookmark1.parentGuid ] },
                  { name: "onItemChanged",
                    arguments: [ (yield PlacesUtils.promiseItemId(bookmark2.guid)),
                                 "keyword", false, "keyword",
                                 bookmark2.lastModified, bookmark2.type,
                                 (yield PlacesUtils.promiseItemId(bookmark2.parentGuid)),
                                 bookmark2.guid, bookmark2.parentGuid ] } ]);

  yield check_keyword(false, "http://example1.com/", "keyword");
  Assert.equal((yield foreign_count("http://example1.com/")), fc1 + 1); 
  yield check_keyword(true, "http://example2.com/", "keyword");
  Assert.equal((yield foreign_count("http://example2.com/")), fc2 + 2); 

  
  observer = expectBookmarkNotifications();
  yield PlacesUtils.keywords.remove("keyword");
  observer.check([{ name: "onItemChanged",
                    arguments: [ (yield PlacesUtils.promiseItemId(bookmark2.guid)),
                                 "keyword", false, "",
                                 bookmark2.lastModified, bookmark2.type,
                                 (yield PlacesUtils.promiseItemId(bookmark2.parentGuid)),
                                 bookmark2.guid, bookmark2.parentGuid ] } ]);

  yield check_keyword(false, "http://example1.com/", "keyword");
  yield check_keyword(false, "http://example2.com/", "keyword");
  Assert.equal((yield foreign_count("http://example1.com/")), fc1 + 1);
  Assert.equal((yield foreign_count("http://example2.com/")), fc2 + 1); 

  yield PlacesUtils.bookmarks.remove(bookmark1);
  yield PlacesUtils.bookmarks.remove(bookmark2);
  Assert.equal((yield foreign_count("http://example1.com/")), fc1); 
  while ((yield foreign_count("http://example2.com/"))); 

  yield check_no_orphans();
});

add_task(function* test_sameURIDifferentKeyword() {
  let fc = yield foreign_count("http://example.com/");

  let observer = expectBookmarkNotifications();
  let bookmark = yield PlacesUtils.bookmarks.insert({ url: "http://example.com/",
                                                      type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                      parentGuid: PlacesUtils.bookmarks.unfiledGuid });
  yield PlacesUtils.keywords.insert({keyword: "keyword", url: "http://example.com/" });

  yield check_keyword(true, "http://example.com/", "keyword");
  Assert.equal((yield foreign_count("http://example.com/")), fc + 2); 

  observer.check([{ name: "onItemChanged",
                    arguments: [ (yield PlacesUtils.promiseItemId(bookmark.guid)),
                                 "keyword", false, "keyword",
                                 bookmark.lastModified, bookmark.type,
                                 (yield PlacesUtils.promiseItemId(bookmark.parentGuid)),
                                 bookmark.guid, bookmark.parentGuid ] } ]);

  observer = expectBookmarkNotifications();
  yield PlacesUtils.keywords.insert({ keyword: "keyword2", url: "http://example.com/" });
  yield check_keyword(true, "http://example.com/", "keyword");
  yield check_keyword(true, "http://example.com/", "keyword2");
  Assert.equal((yield foreign_count("http://example.com/")), fc + 3); 
  observer.check([{ name: "onItemChanged",
                    arguments: [ (yield PlacesUtils.promiseItemId(bookmark.guid)),
                                 "keyword", false, "keyword2",
                                 bookmark.lastModified, bookmark.type,
                                 (yield PlacesUtils.promiseItemId(bookmark.parentGuid)),
                                 bookmark.guid, bookmark.parentGuid ] } ]);

  
  yield PlacesUtils.keywords.insert({ keyword: "keyword3", url: "http://example.com/" });
  yield check_keyword(true, "http://example.com/", "keyword");
  yield check_keyword(true, "http://example.com/", "keyword2");
  yield check_keyword(true, "http://example.com/", "keyword3");
  Assert.equal((yield foreign_count("http://example.com/")), fc + 4); 

  
  observer = expectBookmarkNotifications();
  yield PlacesUtils.keywords.remove("keyword");
  yield check_keyword(false, "http://example.com/", "keyword");
  yield check_keyword(true, "http://example.com/", "keyword2");
  yield check_keyword(true, "http://example.com/", "keyword3");
  observer.check([{ name: "onItemChanged",
                    arguments: [ (yield PlacesUtils.promiseItemId(bookmark.guid)),
                                 "keyword", false, "",
                                 bookmark.lastModified, bookmark.type,
                                 (yield PlacesUtils.promiseItemId(bookmark.parentGuid)),
                                 bookmark.guid, bookmark.parentGuid ] } ]);
  Assert.equal((yield foreign_count("http://example.com/")), fc + 3); 

  
  yield PlacesUtils.bookmarks.remove(bookmark);
  while ((yield foreign_count("http://example.com/")));
  yield check_keyword(false, "http://example.com/", "keyword");
  yield check_keyword(false, "http://example.com/", "keyword2");
  yield check_keyword(false, "http://example.com/", "keyword3");

  check_no_orphans();
});

add_task(function* test_deleteKeywordMultipleBookmarks() {
  let fc = yield foreign_count("http://example.com/");

  let observer = expectBookmarkNotifications();
  let bookmark1 = yield PlacesUtils.bookmarks.insert({ url: "http://example.com/",
                                                       type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                       parentGuid: PlacesUtils.bookmarks.unfiledGuid });
  let bookmark2 = yield PlacesUtils.bookmarks.insert({ url: "http://example.com/",
                                                       type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                       parentGuid: PlacesUtils.bookmarks.unfiledGuid });
  yield PlacesUtils.keywords.insert({ keyword: "keyword", url: "http://example.com/" });

  yield check_keyword(true, "http://example.com/", "keyword");
  Assert.equal((yield foreign_count("http://example.com/")), fc + 3); 
  observer.check([{ name: "onItemChanged",
                    arguments: [ (yield PlacesUtils.promiseItemId(bookmark2.guid)),
                                 "keyword", false, "keyword",
                                 bookmark2.lastModified, bookmark2.type,
                                 (yield PlacesUtils.promiseItemId(bookmark2.parentGuid)),
                                 bookmark2.guid, bookmark2.parentGuid ] },
                  { name: "onItemChanged",
                    arguments: [ (yield PlacesUtils.promiseItemId(bookmark1.guid)),
                                 "keyword", false, "keyword",
                                 bookmark1.lastModified, bookmark1.type,
                                 (yield PlacesUtils.promiseItemId(bookmark1.parentGuid)),
                                 bookmark1.guid, bookmark1.parentGuid ] } ]);

  observer = expectBookmarkNotifications();
  yield PlacesUtils.keywords.remove("keyword");
  yield check_keyword(false, "http://example.com/", "keyword");
  Assert.equal((yield foreign_count("http://example.com/")), fc + 2); 
  observer.check([{ name: "onItemChanged",
                    arguments: [ (yield PlacesUtils.promiseItemId(bookmark2.guid)),
                                 "keyword", false, "",
                                 bookmark2.lastModified, bookmark2.type,
                                 (yield PlacesUtils.promiseItemId(bookmark2.parentGuid)),
                                 bookmark2.guid, bookmark2.parentGuid ] },
                  { name: "onItemChanged",
                    arguments: [ (yield PlacesUtils.promiseItemId(bookmark1.guid)),
                                 "keyword", false, "",
                                 bookmark1.lastModified, bookmark1.type,
                                 (yield PlacesUtils.promiseItemId(bookmark1.parentGuid)),
                                 bookmark1.guid, bookmark1.parentGuid ] } ]);

  
  yield PlacesUtils.bookmarks.remove(bookmark1);
  yield PlacesUtils.bookmarks.remove(bookmark2);
  Assert.equal((yield foreign_count("http://example.com/")), fc); 

  check_no_orphans();
});

add_task(function* test_multipleKeywordsSamePostData() {
  yield PlacesUtils.keywords.insert({ keyword: "keyword", url: "http://example.com/", postData: "postData1" });
  yield check_keyword(true, "http://example.com/", "keyword", "postData1");
  
  yield Assert.rejects(PlacesUtils.keywords.insert({ keyword: "keyword2", url: "http://example.com/", postData: "postData1" }),
                       /constraint failed/);
  yield check_keyword(false, "http://example.com/", "keyword2", "postData1");

  yield PlacesUtils.keywords.remove("keyword");

  check_no_orphans();
});

add_task(function* test_oldPostDataAPI() {
  let bookmark = yield PlacesUtils.bookmarks.insert({ url: "http://example.com/",
                                                      type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                      parentGuid: PlacesUtils.bookmarks.unfiledGuid });
  yield PlacesUtils.keywords.insert({ keyword: "keyword", url: "http://example.com/" });
  let itemId = yield PlacesUtils.promiseItemId(bookmark.guid);
  yield PlacesUtils.setPostDataForBookmark(itemId, "postData");
  yield check_keyword(true, "http://example.com/", "keyword", "postData");
  Assert.equal(PlacesUtils.getPostDataForBookmark(itemId), "postData");

  yield PlacesUtils.keywords.remove("keyword");
  yield PlacesUtils.bookmarks.remove(bookmark);

  check_no_orphans();
});

add_task(function* test_oldKeywordsAPI() {
  let bookmark = yield PlacesUtils.bookmarks.insert({ url: "http://example.com/",
                                                    type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
                                                    parentGuid: PlacesUtils.bookmarks.unfiledGuid });
  yield check_keyword(false, "http://example.com/", "keyword");
  let itemId = yield PlacesUtils.promiseItemId(bookmark.guid);

  PlacesUtils.bookmarks.setKeywordForBookmark(itemId, "keyword");
  yield promiseKeyword("keyword", "http://example.com/");

  
  PlacesUtils.bookmarks.setKeywordForBookmark(itemId, "");
  yield promiseKeyword("keyword", null);

  yield PlacesUtils.keywords.insert({ keyword: "keyword", url: "http://example.com" });
  Assert.equal(PlacesUtils.bookmarks.getKeywordForBookmark(itemId), "keyword");
  Assert.equal(PlacesUtils.bookmarks.getURIForKeyword("keyword").spec, "http://example.com/");
  yield PlacesUtils.bookmarks.remove(bookmark);

  check_no_orphans();
});

function run_test() {
  run_next_test();
}
