








"use strict";




XPCOMUtils.defineLazyModuleGetter(this, "DownloadStore",
                                  "resource://gre/modules/DownloadStore.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm")













function promiseNewListAndStore(aStorePath) {
  return promiseNewDownloadList().then(function (aList) {
    let path = aStorePath || getTempFile(TEST_STORE_FILE_NAME).path;
    let store = new DownloadStore(aList, path);
    return [aList, store];
  });
}







add_task(function test_save_reload()
{
  let [listForSave, storeForSave] = yield promiseNewListAndStore();
  let [listForLoad, storeForLoad] = yield promiseNewListAndStore(
                                                 storeForSave.path);

  listForSave.add(yield promiseSimpleDownload(httpUrl("source.txt")));
  listForSave.add(yield Downloads.createDownload({
    source: { url: httpUrl("empty.txt"),
              referrer: TEST_REFERRER_URL },
    target: getTempFile(TEST_TARGET_FILE_NAME),
  }));

  yield storeForSave.save();
  yield storeForLoad.load();

  let itemsForSave = yield listForSave.getAll();
  let itemsForLoad = yield listForLoad.getAll();

  do_check_eq(itemsForSave.length, itemsForLoad.length);

  
  for (let i = 0; i < itemsForSave.length; i++) {
    
    do_check_neq(itemsForSave[i], itemsForLoad[i]);

    
    do_check_eq(itemsForSave[i].source.url,
                itemsForLoad[i].source.url);
    do_check_eq(itemsForSave[i].source.referrer,
                itemsForLoad[i].source.referrer);
    do_check_eq(itemsForSave[i].target.path,
                itemsForLoad[i].target.path);
    do_check_eq(itemsForSave[i].saver.type,
                itemsForLoad[i].saver.type);
  }
});




add_task(function test_save_empty()
{
  let [list, store] = yield promiseNewListAndStore();

  let createdFile = yield OS.File.open(store.path, { create: true });
  yield createdFile.close();

  yield store.save();

  do_check_false(yield OS.File.exists(store.path));

  
  yield store.save();
});




add_task(function test_load_empty()
{
  let [list, store] = yield promiseNewListAndStore();

  do_check_false(yield OS.File.exists(store.path));

  yield store.load();

  let items = yield list.getAll();
  do_check_eq(items.length, 0);
});






add_task(function test_load_string_predefined()
{
  let [list, store] = yield promiseNewListAndStore();

  
  let targetPath = getTempFile(TEST_TARGET_FILE_NAME).path;
  let filePathLiteral = JSON.stringify(targetPath);
  let sourceUriLiteral = JSON.stringify(httpUrl("source.txt"));
  let emptyUriLiteral = JSON.stringify(httpUrl("empty.txt"));
  let referrerUriLiteral = JSON.stringify(TEST_REFERRER_URL);

  let string = "{\"list\":[{\"source\":" + sourceUriLiteral + "," +
                "\"target\":" + filePathLiteral + "}," +
                "{\"source\":{\"url\":" + emptyUriLiteral + "," +
                "\"referrer\":" + referrerUriLiteral + "}," +
                "\"target\":" + filePathLiteral + "}]}";

  yield OS.File.writeAtomic(store.path,
                            new TextEncoder().encode(string),
                            { tmpPath: store.path + ".tmp" });

  yield store.load();

  let items = yield list.getAll();

  do_check_eq(items.length, 2);

  do_check_eq(items[0].source.url, httpUrl("source.txt"));
  do_check_eq(items[0].target.path, targetPath);

  do_check_eq(items[1].source.url, httpUrl("empty.txt"));
  do_check_eq(items[1].source.referrer, TEST_REFERRER_URL);
  do_check_eq(items[1].target.path, targetPath);
});




add_task(function test_load_string_unrecognized()
{
  let [list, store] = yield promiseNewListAndStore();

  
  let targetPath = getTempFile(TEST_TARGET_FILE_NAME).path;
  let filePathLiteral = JSON.stringify(targetPath);
  let sourceUriLiteral = JSON.stringify(httpUrl("source.txt"));

  let string = "{\"list\":[{\"source\":null," +
                "\"target\":null}," +
                "{\"source\":{\"url\":" + sourceUriLiteral + "}," +
                "\"target\":{\"path\":" + filePathLiteral + "}," +
                "\"saver\":{\"type\":\"copy\"}}]}";

  yield OS.File.writeAtomic(store.path,
                            new TextEncoder().encode(string),
                            { tmpPath: store.path + ".tmp" });

  yield store.load();

  let items = yield list.getAll();

  do_check_eq(items.length, 1);

  do_check_eq(items[0].source.url, httpUrl("source.txt"));
  do_check_eq(items[0].target.path, targetPath);
});




add_task(function test_load_string_malformed()
{
  let [list, store] = yield promiseNewListAndStore();

  let string = "{\"list\":[{\"source\":null,\"target\":null}," +
                "{\"source\":{\"url\":\"about:blank\"}}}";

  yield OS.File.writeAtomic(store.path, new TextEncoder().encode(string),
                            { tmpPath: store.path + ".tmp" });

  try {
    yield store.load();
    do_throw("Exception expected when JSON data is malformed.");
  } catch (ex if ex.name == "SyntaxError") {
    do_print("The expected SyntaxError exception was thrown.");
  }

  let items = yield list.getAll();

  do_check_eq(items.length, 0);
});
