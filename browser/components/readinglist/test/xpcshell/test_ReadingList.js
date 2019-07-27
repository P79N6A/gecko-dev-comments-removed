


"use strict";

let gDBFile = do_get_profile();

Cu.import("resource:///modules/readinglist/ReadingList.jsm");
Cu.import("resource:///modules/readinglist/SQLiteStore.jsm");
Cu.import("resource://gre/modules/Sqlite.jsm");
Cu.import("resource://gre/modules/Timer.jsm");

var gList;
var gItems;

function run_test() {
  run_next_test();
}

add_task(function* prepare() {
  gList = ReadingList;
  Assert.ok(gList);
  gDBFile.append(gList._store.pathRelativeToProfileDir);
  do_register_cleanup(function* () {
    
    yield gList.destroy();
    if (gDBFile.exists()) {
      gDBFile.remove(true);
    }
  });

  gItems = [];
  for (let i = 0; i < 3; i++) {
    gItems.push({
      guid: `guid${i}`,
      url: `http://example.com/${i}`,
      resolvedURL: `http://example.com/resolved/${i}`,
      title: `title ${i}`,
      excerpt: `excerpt ${i}`,
      unread: 0,
      favorite: 0,
      isArticle: 1,
      storedOn: Date.now(),
    });
  }

  for (let item of gItems) {
    let addedItem = yield gList.addItem(item);
    checkItems(addedItem, item);
  }
});

add_task(function* item_properties() {
  
  let iter = gList.iterator({
    sort: "guid",
  });
  let item = (yield iter.items(1))[0];
  Assert.ok(item);

  Assert.ok(item.uri);
  Assert.ok(item.uri instanceof Ci.nsIURI);
  Assert.equal(item.uri.spec, item._record.url);

  Assert.ok(item.resolvedURI);
  Assert.ok(item.resolvedURI instanceof Ci.nsIURI);
  Assert.equal(item.resolvedURI.spec, item._record.resolvedURL);

  Assert.ok(item.addedOn);
  Assert.ok(item.addedOn instanceof Cu.getGlobalForObject(ReadingList).Date);

  Assert.ok(item.storedOn);
  Assert.ok(item.storedOn instanceof Cu.getGlobalForObject(ReadingList).Date);

  Assert.ok(typeof(item.favorite) == "boolean");
  Assert.ok(typeof(item.isArticle) == "boolean");
  Assert.ok(typeof(item.unread) == "boolean");

  Assert.equal(item.id, hash(item._record.url));
});

add_task(function* constraints() {
  
  let err = null;
  try {
    yield gList.addItem(gItems[0]);
  }
  catch (e) {
    err = e;
  }
  checkError(err);

  
  let item = kindOfClone(gItems[0]);
  item.guid = gItems[0].guid;
  err = null;
  try {
    yield gList.addItem(item);
  }
  catch (e) {
    err = e;
  }
  checkError(err);

  
  item = kindOfClone(gItems[0]);
  item.url = gItems[0].url;
  err = null;
  try {
    yield gList.addItem(item);
  }
  catch (e) {
    err = e;
  }
  checkError(err);

  
  item = kindOfClone(gItems[0]);
  item.resolvedURL = gItems[0].resolvedURL;
  err = null;
  try {
    yield gList.addItem(item);
  }
  catch (e) {
    err = e;
  }
  checkError(err);

  
  item = kindOfClone(gItems[0]);
  delete item.url;
  err = null;
  try {
    yield gList.addItem(item);
  }
  catch (e) {
    err = e;
  }
  Assert.ok(err);
  Assert.ok(err instanceof Cu.getGlobalForObject(ReadingList).Error, err);
  Assert.equal(err.message, "The item must have a url");

  
  item = (yield gList.item({ guid: gItems[0].guid }));
  Assert.ok(item);
  let oldURL = item._record.url;
  item._record.url = null;
  err = null;
  try {
    yield gList.updateItem(item);
  }
  catch (e) {
    err = e;
  }
  item._record.url = oldURL;
  Assert.ok(err);
  Assert.ok(err instanceof Cu.getGlobalForObject(ReadingList).Error, err);
  Assert.equal(err.message, "The item must have a url");

  
  item = kindOfClone(gItems[0]);
  item.bogus = "gnarly";
  err = null;
  try {
    yield gList.addItem(item);
  }
  catch (e) {
    err = e;
  }
  Assert.ok(err);
  Assert.ok(err.message);
  Assert.ok(err.message.indexOf("Unrecognized item property:") >= 0);

  
  item = kindOfClone(gItems[0]);
  delete item.guid;
  err = null;
  let rlitem1;
  try {
    rlitem1 = yield gList.addItem(item);
  }
  catch (e) {
    err = e;
  }
  Assert.ok(!err, err ? err.message : undefined);

  
  item = kindOfClone(gItems[1]);
  delete item.guid;
  err = null;
  let rlitem2;
  try {
    rlitem2 = yield gList.addItem(item);
  }
  catch (e) {
    err = e;
  }
  Assert.ok(!err, err ? err.message : undefined);

  
  
  yield gList.deleteItem(rlitem1);
  yield gList.deleteItem(rlitem2);
  let items = [];
  yield gList.forEachItem(i => items.push(i), { url: [rlitem1.uri.spec, rlitem2.uri.spec] });
  Assert.equal(items.length, 0);
});

add_task(function* count() {
  let count = yield gList.count();
  Assert.equal(count, gItems.length);

  count = yield gList.count({
    guid: gItems[0].guid,
  });
  Assert.equal(count, 1);
});

add_task(function* forEachItem() {
  
  let items = [];
  yield gList.forEachItem(item => items.push(item), {
    sort: "guid",
  });
  checkItems(items, gItems);

  
  items = [];
  yield gList.forEachItem(item => items.push(item), {
    limit: 1,
    sort: "guid",
  });
  checkItems(items, gItems.slice(0, 1));

  
  items = [];
  yield gList.forEachItem(item => items.push(item), {
    limit: 1,
    sort: "guid",
    descending: true,
  });
  checkItems(items, gItems.slice(gItems.length - 1, gItems.length));

  
  items = [];
  yield gList.forEachItem(item => items.push(item), {
    guid: gItems[0].guid,
  });
  checkItems(items, gItems.slice(0, 1));

  
  items = [];
  yield gList.forEachItem(item => items.push(item), {
    guid: gItems.map(i => i.guid),
    sort: "guid",
  });
  checkItems(items, gItems);

  
  items = [];
  yield gList.forEachItem(item => items.push(item), {
    guid: gItems.map(i => i.guid),
    title: gItems[0].title,
    sort: "guid",
  });
  checkItems(items, [gItems[0]]);

  
  items = [];
  yield gList.forEachItem(item => items.push(item), {
    guid: gItems[1].guid,
    sort: "guid",
  }, {
    guid: gItems[0].guid,
  });
  checkItems(items, [gItems[0], gItems[1]]);

  
  items = [];
  yield gList.forEachItem(item => items.push(item), {
    guid: gItems.map(i => i.guid),
    title: gItems[1].title,
    sort: "guid",
  }, {
    guid: gItems[0].guid,
  });
  checkItems(items, [gItems[0], gItems[1]]);
});

add_task(function* forEachSyncedDeletedItem() {
  let deletedItem = yield gList.addItem({
    guid: "forEachSyncedDeletedItem",
    url: "http://example.com/forEachSyncedDeletedItem",
  });
  deletedItem._record.syncStatus = gList.SyncStatus.SYNCED;
  yield gList.deleteItem(deletedItem);
  let items = [];
  yield gList.forEachSyncedDeletedItem(item => items.push(item));
  Assert.equal(items.length, 1);
  Assert.equal(items[0].guid, deletedItem.guid);
});

add_task(function* forEachItem_promises() {
  
  let items = [];
  yield gList.forEachItem(item => {
    items.push(item);
    return Promise.resolve();
  }, {
    sort: "guid",
  });
  checkItems(items, gItems);

  
  items = [];
  let i = 0;
  let promises = [];
  yield gList.forEachItem(item => {
    items.push(item);
    
    if (i > 0) {
      Assert.equal(promises[i - 1], null);
    }
    
    let this_i = i++;
    let promise = new Promise(resolve => {
      
      
      
      
      setTimeout(() => {
        promises[this_i] = null;
        resolve();
      }, 0);
    });
    promises.push(promise);
    return promise;
  }, {
    sort: "guid",
  });
  checkItems(items, gItems);
});

add_task(function* iterator_forEach() {
  
  let items = [];
  let iter = gList.iterator({
    sort: "guid",
  });
  yield iter.forEach(item => items.push(item));
  checkItems(items, gItems);

  
  items = [];
  iter = gList.iterator({
    sort: "guid",
  });
  for (let i = 0; i < gItems.length; i++) {
    yield iter.forEach(item => items.push(item), 1);
    checkItems(items, gItems.slice(0, i + 1));
  }
  yield iter.forEach(item => items.push(item), 100);
  checkItems(items, gItems);
  yield iter.forEach(item => items.push(item));
  checkItems(items, gItems);

  
  items = [];
  iter = gList.iterator({
    sort: "guid",
    guid: gItems[0].guid,
  });
  yield iter.forEach(item => items.push(item));
  checkItems(items, [gItems[0]]);

  
  items = [];
  iter = gList.iterator({
    sort: "guid",
    guid: gItems.map(i => i.guid),
  });
  yield iter.forEach(item => items.push(item));
  checkItems(items, gItems);

  
  items = [];
  iter = gList.iterator({
    sort: "guid",
    guid: gItems.map(i => i.guid),
    title: gItems[0].title,
  });
  yield iter.forEach(item => items.push(item));
  checkItems(items, [gItems[0]]);

  
  items = [];
  iter = gList.iterator({
    sort: "guid",
    guid: gItems[1].guid,
  }, {
    guid: gItems[0].guid,
  });
  yield iter.forEach(item => items.push(item));
  checkItems(items, [gItems[0], gItems[1]]);

  
  items = [];
  iter = gList.iterator({
    sort: "guid",
    guid: gItems.map(i => i.guid),
    title: gItems[1].title,
  }, {
    guid: gItems[0].guid,
  });
  yield iter.forEach(item => items.push(item));
  checkItems(items, [gItems[0], gItems[1]]);
});

add_task(function* iterator_items() {
  
  let iter = gList.iterator({
    sort: "guid",
  });
  let items = yield iter.items(gItems.length);
  checkItems(items, gItems);
  items = yield iter.items(100);
  checkItems(items, []);

  
  iter = gList.iterator({
    sort: "guid",
  });
  for (let i = 0; i < gItems.length; i++) {
    items = yield iter.items(1);
    checkItems(items, gItems.slice(i, i + 1));
  }
  items = yield iter.items(100);
  checkItems(items, []);

  
  iter = gList.iterator({
    sort: "guid",
    guid: gItems[0].guid,
  });
  items = yield iter.items(gItems.length);
  checkItems(items, [gItems[0]]);

  
  iter = gList.iterator({
    sort: "guid",
    guid: gItems.map(i => i.guid),
  });
  items = yield iter.items(gItems.length);
  checkItems(items, gItems);

  
  iter = gList.iterator({
    sort: "guid",
    guid: gItems.map(i => i.guid),
    title: gItems[0].title,
  });
  items = yield iter.items(gItems.length);
  checkItems(items, [gItems[0]]);

  
  iter = gList.iterator({
    sort: "guid",
    guid: gItems[1].guid,
  }, {
    guid: gItems[0].guid,
  });
  items = yield iter.items(gItems.length);
  checkItems(items, [gItems[0], gItems[1]]);

  
  iter = gList.iterator({
    sort: "guid",
    guid: gItems.map(i => i.guid),
    title: gItems[1].title,
  }, {
    guid: gItems[0].guid,
  });
  items = yield iter.items(gItems.length);
  checkItems(items, [gItems[0], gItems[1]]);
});

add_task(function* iterator_forEach_promise() {
  
  let items = [];
  let iter = gList.iterator({
    sort: "guid",
  });
  yield iter.forEach(item => {
    items.push(item);
    return Promise.resolve();
  });
  checkItems(items, gItems);

  
  
  items = [];
  let i = 0;
  let promises = [];
  iter = gList.iterator({
    sort: "guid",
  });
  yield iter.forEach(item => {
    items.push(item);
    if (i > 0) {
      Assert.equal(promises[i - 1], null);
    }
    let this_i = i++;
    let promise = new Promise(resolve => {
      setTimeout(() => {
        promises[this_i] = null;
        resolve();
      }, 0);
    });
    promises.push(promise);
    return promise;
  });
  checkItems(items, gItems);
});

add_task(function* item() {
  let item = yield gList.item({ guid: gItems[0].guid });
  checkItems([item], [gItems[0]]);

  item = yield gList.item({ guid: gItems[1].guid });
  checkItems([item], [gItems[1]]);
});

add_task(function* itemForURL() {
  let item = yield gList.itemForURL(gItems[0].url);
  checkItems([item], [gItems[0]]);

  item = yield gList.itemForURL(gItems[1].url);
  checkItems([item], [gItems[1]]);
});

add_task(function* updateItem() {
  
  let items = [];
  yield gList.forEachItem(i => items.push(i), {
    guid: gItems[0].guid,
  });
  Assert.equal(items.length, 1);
  let item = items[0];

  
  let newTitle = "updateItem new title";
  Assert.notEqual(item.title, newTitle);
  item.title = newTitle;
  yield gList.updateItem(item);

  
  items = [];
  yield gList.forEachItem(i => items.push(i), {
    guid: gItems[0].guid,
  });
  Assert.equal(items.length, 1);
  item = items[0];
  Assert.equal(item.title, newTitle);
});

add_task(function* item_setRecord() {
  
  let iter = gList.iterator({
    sort: "guid",
  });
  let item = (yield iter.items(1))[0];
  Assert.ok(item);

  
  
  let newTitle = "item_setRecord title 1";
  item._record.title = newTitle;
  yield gList.updateItem(item);
  Assert.equal(item.title, newTitle);
  iter = gList.iterator({
    sort: "guid",
  });
  let sameItem = (yield iter.items(1))[0];
  Assert.ok(item === sameItem);
  Assert.equal(sameItem.title, newTitle);

  
  
  newTitle = "item_setRecord title 2";
  item.title = newTitle;
  yield gList.updateItem(item);
  Assert.equal(item.title, newTitle);
  iter = gList.iterator({
    sort: "guid",
  });
  sameItem = (yield iter.items(1))[0];
  Assert.ok(item === sameItem);
  Assert.equal(sameItem.title, newTitle);

  
  let err = null;
  try {
    item._record = { bogus: "gnarly" };
  }
  catch (e) {
    err = e;
  }
  Assert.ok(err);
  Assert.ok(err.message);
  Assert.ok(err.message.indexOf("Unrecognized item property:") >= 0);
});

add_task(function* listeners() {
  Assert.equal((yield gList.count()), gItems.length);
  
  let resolve;
  let listenerPromise = new Promise(r => resolve = r);
  let listener = {
    onItemAdded: resolve,
  };
  gList.addListener(listener);
  let item = kindOfClone(gItems[0]);
  let items = yield Promise.all([listenerPromise, gList.addItem(item)]);
  Assert.ok(items[0]);
  Assert.ok(items[0] === items[1]);
  gList.removeListener(listener);
  Assert.equal((yield gList.count()), gItems.length + 1);

  
  listenerPromise = new Promise(r => resolve = r);
  listener = {
    onItemUpdated: resolve,
  };
  gList.addListener(listener);
  items[0].title = "listeners new title";
  yield gList.updateItem(items[0]);
  let listenerItem = yield listenerPromise;
  Assert.ok(listenerItem);
  Assert.ok(listenerItem === items[0]);
  gList.removeListener(listener);
  Assert.equal((yield gList.count()), gItems.length + 1);

  
  listenerPromise = new Promise(r => resolve = r);
  listener = {
    onItemDeleted: resolve,
  };
  gList.addListener(listener);
  items[0].delete();
  listenerItem = yield listenerPromise;
  Assert.ok(listenerItem);
  Assert.ok(listenerItem === items[0]);
  gList.removeListener(listener);
  Assert.equal((yield gList.count()), gItems.length);
});


add_task(function* deleteItem() {
  
  let iter = gList.iterator({
    sort: "guid",
  });
  let item = (yield iter.items(1))[0];
  Assert.ok(item);
  item.delete();
  gItems[0].list = null;
  Assert.equal((yield gList.count()), gItems.length - 1);
  let items = [];
  yield gList.forEachItem(i => items.push(i), {
    sort: "guid",
  });
  checkItems(items, gItems.slice(1));

  
  yield gList.deleteItem(items[0]);
  gItems[1].list = null;
  Assert.equal((yield gList.count()), gItems.length - 2);
  items = [];
  yield gList.forEachItem(i => items.push(i), {
    sort: "guid",
  });
  checkItems(items, gItems.slice(2));

  
  yield gList.deleteItem(items[0]);
  gItems[2].list = null;
  Assert.equal((yield gList.count()), gItems.length - 3);
  items = [];
  yield gList.forEachItem(i => items.push(i), {
    sort: "guid",
  });
  checkItems(items, gItems.slice(3));
});

function checkItems(actualItems, expectedItems) {
  Assert.equal(actualItems.length, expectedItems.length);
  for (let i = 0; i < expectedItems.length; i++) {
    for (let prop in expectedItems[i]._record) {
      Assert.ok(prop in actualItems[i]._record, prop);
      Assert.equal(actualItems[i]._record[prop], expectedItems[i][prop]);
    }
  }
}

function checkError(err) {
  Assert.ok(err);
  Assert.ok(err instanceof Cu.getGlobalForObject(Sqlite).Error, err);
}

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

function hash(str) {
  let hasher = Cc["@mozilla.org/security/hash;1"].
               createInstance(Ci.nsICryptoHash);
  hasher.init(Ci.nsICryptoHash.MD5);
  let stream = Cc["@mozilla.org/io/string-input-stream;1"].
               createInstance(Ci.nsIStringInputStream);
  stream.data = str;
  hasher.updateFromStream(stream, -1);
  let binaryStr = hasher.finish(false);
  let hexStr =
    [("0" + binaryStr.charCodeAt(i).toString(16)).slice(-2) for (i in binaryStr)].
    join("");
  return hexStr;
}
