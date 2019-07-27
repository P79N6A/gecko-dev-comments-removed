


"use strict";

Cu.import("resource:///modules/readinglist/SQLiteStore.jsm");
Cu.import("resource://gre/modules/Sqlite.jsm");

var gStore;
var gItems;

function run_test() {
  run_next_test();
}

add_task(function* prepare() {
  let basename = "reading-list-test.sqlite";
  let dbFile = do_get_profile();
  dbFile.append(basename);
  function removeDB() {
    if (dbFile.exists()) {
      dbFile.remove(true);
    }
  }
  removeDB();
  do_register_cleanup(removeDB);

  gStore = new SQLiteStore(dbFile.path);

  gItems = [];
  for (let i = 0; i < 3; i++) {
    gItems.push({
      guid: `guid${i}`,
      url: `http://example.com/${i}`,
      resolvedURL: `http://example.com/resolved/${i}`,
      title: `title ${i}`,
      excerpt: `excerpt ${i}`,
      unread: true,
      addedOn: i,
    });
  }

  for (let item of gItems) {
    yield gStore.addItem(item);
  }
});

add_task(function* constraints() {
  
  let err = null;
  try {
    yield gStore.addItem(gItems[0]);
  }
  catch (e) {
    err = e;
  }
  checkError(err, "UNIQUE constraint failed");

  
  function kindOfClone(item) {
    let newItem = {};
    for (let prop in item) {
      newItem[prop] = item[prop];
      if (typeof(newItem[prop]) == "string") {
        newItem[prop] += " -- make this string different";
      }
    }
    return newItem;
  }
  let item = kindOfClone(gItems[0]);
  item.guid = gItems[0].guid;
  err = null;
  try {
    yield gStore.addItem(item);
  }
  catch (e) {
    err = e;
  }
  checkError(err, "UNIQUE constraint failed: items.guid");

  
  item = kindOfClone(gItems[0]);
  item.url = gItems[0].url;
  err = null;
  try {
    yield gStore.addItem(item);
  }
  catch (e) {
    err = e;
  }
  checkError(err, "UNIQUE constraint failed: items.url");

  
  item.guid = gItems[1].guid;
  err = null;
  try {
    yield gStore.updateItem(item);
  }
  catch (e) {
    err = e;
  }
  
  
  
  checkError(err, "UNIQUE constraint failed: items.guid");

  
  item = kindOfClone(gItems[0]);
  item.resolvedURL = gItems[0].resolvedURL;
  err = null;
  try {
    yield gStore.addItem(item);
  }
  catch (e) {
    err = e;
  }
  checkError(err, "UNIQUE constraint failed: items.resolvedURL");

  
  item.url = gItems[1].url;
  err = null;
  try {
    yield gStore.updateItem(item);
  }
  catch (e) {
    err = e;
  }
  checkError(err, "UNIQUE constraint failed: items.resolvedURL");

  
  item = kindOfClone(gItems[0]);
  delete item.guid;
  err = null;
  try {
    yield gStore.addItem(item);
  }
  catch (e) {
    err = e;
  }
  Assert.ok(!err, err ? err.message : undefined);
  let url1 = item.url;

  
  item = kindOfClone(gItems[1]);
  delete item.guid;
  err = null;
  try {
    yield gStore.addItem(item);
  }
  catch (e) {
    err = e;
  }
  Assert.ok(!err, err ? err.message : undefined);
  let url2 = item.url;

  
  yield gStore.deleteItemByURL(url1);
  yield gStore.deleteItemByURL(url2);
  let items = [];
  yield gStore.forEachItem(i => items.push(i), { url: [url1, url2] });
  Assert.equal(items.length, 0);

  
  item = kindOfClone(gItems[0]);
  delete item.url;
  err = null;
  try {
    yield gStore.addItem(item);
  }
  catch (e) {
    err = e;
  }
  checkError(err, "NOT NULL constraint failed: items.url");
});

add_task(function* count() {
  let count = yield gStore.count();
  Assert.equal(count, gItems.length);

  count = yield gStore.count({
    guid: gItems[0].guid,
  });
  Assert.equal(count, 1);
});

add_task(function* forEachItem() {
  
  let items = [];
  yield gStore.forEachItem(item => items.push(item), {
    sort: "guid",
  });
  checkItems(items, gItems);

  
  items = [];
  yield gStore.forEachItem(item => items.push(item), {
    limit: 1,
    sort: "guid",
  });
  checkItems(items, gItems.slice(0, 1));

  
  items = [];
  yield gStore.forEachItem(item => items.push(item), {
    limit: 1,
    sort: "guid",
    descending: true,
  });
  checkItems(items, gItems.slice(gItems.length - 1, gItems.length));

  
  items = [];
  yield gStore.forEachItem(item => items.push(item), {
    guid: gItems[0].guid,
  });
  checkItems(items, gItems.slice(0, 1));

  
  items = [];
  yield gStore.forEachItem(item => items.push(item), {
    guid: gItems.map(i => i.guid),
    sort: "guid",
  });
  checkItems(items, gItems);

  
  items = [];
  yield gStore.forEachItem(item => items.push(item), {
    guid: gItems.map(i => i.guid),
    title: gItems[0].title,
    sort: "guid",
  });
  checkItems(items, [gItems[0]]);

  
  items = [];
  yield gStore.forEachItem(item => items.push(item), {
    guid: gItems[1].guid,
    sort: "guid",
  }, {
    guid: gItems[0].guid,
  });
  checkItems(items, [gItems[0], gItems[1]]);

  
  items = [];
  yield gStore.forEachItem(item => items.push(item), {
    guid: gItems.map(i => i.guid),
    title: gItems[1].title,
    sort: "guid",
  }, {
    guid: gItems[0].guid,
  });
  checkItems(items, [gItems[0], gItems[1]]);
});

add_task(function* updateItem() {
  let newTitle = "a new title";
  gItems[0].title = newTitle;
  yield gStore.updateItem(gItems[0]);
  let item;
  yield gStore.forEachItem(i => item = i, {
    guid: gItems[0].guid,
  });
  Assert.ok(item);
  Assert.equal(item.title, gItems[0].title);
});


add_task(function* deleteItemByURL() {
  
  yield gStore.deleteItemByURL(gItems[0].url);
  Assert.equal((yield gStore.count()), gItems.length - 1);
  let items = [];
  yield gStore.forEachItem(i => items.push(i), {
    sort: "guid",
  });
  checkItems(items, gItems.slice(1));

  
  yield gStore.deleteItemByURL(gItems[1].url);
  Assert.equal((yield gStore.count()), gItems.length - 2);
  items = [];
  yield gStore.forEachItem(i => items.push(i), {
    sort: "guid",
  });
  checkItems(items, gItems.slice(2));

  
  yield gStore.deleteItemByURL(gItems[2].url);
  Assert.equal((yield gStore.count()), gItems.length - 3);
  items = [];
  yield gStore.forEachItem(i => items.push(i), {
    sort: "guid",
  });
  checkItems(items, gItems.slice(3));
});

function checkItems(actualItems, expectedItems) {
  Assert.equal(actualItems.length, expectedItems.length);
  for (let i = 0; i < expectedItems.length; i++) {
    for (let prop in expectedItems[i]) {
      Assert.ok(prop in actualItems[i], prop);
      Assert.equal(actualItems[i][prop], expectedItems[i][prop]);
    }
  }
}

function checkError(err, expectedMsgSubstring) {
  Assert.ok(err);
  Assert.ok(err instanceof Cu.getGlobalForObject(Sqlite).Error);
  Assert.ok(err.message);
  Assert.ok(err.message.indexOf(expectedMsgSubstring) >= 0, err.message);
}
