








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

  listForSave.add(yield promiseSimpleDownload(TEST_SOURCE_URI));
  listForSave.add(yield Downloads.createDownload({
    source: { uri: TEST_EMPTY_URI,
              referrer: TEST_REFERRER_URI },
    target: { file: getTempFile(TEST_TARGET_FILE_NAME) },
    saver: { type: "copy" },
  }));

  yield storeForSave.save();
  yield storeForLoad.load();

  let itemsForSave = yield listForSave.getAll();
  let itemsForLoad = yield listForLoad.getAll();

  do_check_eq(itemsForSave.length, itemsForLoad.length);

  
  for (let i = 0; i < itemsForSave.length; i++) {
    
    do_check_neq(itemsForSave[i], itemsForLoad[i]);

    
    do_check_true(itemsForSave[i].source.uri.equals(
                  itemsForLoad[i].source.uri));
    if (itemsForSave[i].source.referrer) {
      do_check_true(itemsForSave[i].source.referrer.equals(
                    itemsForLoad[i].source.referrer));
    } else {
      do_check_true(itemsForLoad[i].source.referrer === null);
    }
    do_check_true(itemsForSave[i].target.file.equals(
                  itemsForLoad[i].target.file));
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

  
  let targetFile = getTempFile(TEST_TARGET_FILE_NAME);
  let filePathLiteral = JSON.stringify(targetFile.path);
  let sourceUriLiteral = JSON.stringify(TEST_SOURCE_URI.spec);
  let emptyUriLiteral = JSON.stringify(TEST_EMPTY_URI.spec);
  let referrerUriLiteral = JSON.stringify(TEST_REFERRER_URI.spec);

  let string = "[{\"source\":{\"uri\":" + sourceUriLiteral + "}," +
                "\"target\":{\"file\":" + filePathLiteral + "}," +
                "\"saver\":{\"type\":\"copy\"}}," +
                "{\"source\":{\"uri\":" + emptyUriLiteral + "," +
                "\"referrer\":" + referrerUriLiteral + "}," +
                "\"target\":{\"file\":" + filePathLiteral + "}," +
                "\"saver\":{\"type\":\"copy\"}}]";

  yield OS.File.writeAtomic(store.path,
                            new TextEncoder().encode(string),
                            { tmpPath: store.path + ".tmp" });

  yield store.load();

  let items = yield list.getAll();

  do_check_eq(items.length, 2);

  do_check_true(items[0].source.uri.equals(TEST_SOURCE_URI));
  do_check_true(items[0].target.file.equals(targetFile));

  do_check_true(items[1].source.uri.equals(TEST_EMPTY_URI));
  do_check_true(items[1].source.referrer.equals(TEST_REFERRER_URI));
  do_check_true(items[1].target.file.equals(targetFile));
});




add_task(function test_load_string_unrecognized()
{
  let [list, store] = yield promiseNewListAndStore();

  
  let targetFile = getTempFile(TEST_TARGET_FILE_NAME);
  let filePathLiteral = JSON.stringify(targetFile.path);
  let sourceUriLiteral = JSON.stringify(TEST_SOURCE_URI.spec);

  let string = "[{\"source\":null," +
                "\"target\":null}," +
                "{\"source\":{\"uri\":" + sourceUriLiteral + "}," +
                "\"target\":{\"file\":" + filePathLiteral + "}," +
                "\"saver\":{\"type\":\"copy\"}}]";

  yield OS.File.writeAtomic(store.path,
                            new TextEncoder().encode(string),
                            { tmpPath: store.path + ".tmp" });

  yield store.load();

  let items = yield list.getAll();

  do_check_eq(items.length, 1);

  do_check_true(items[0].source.uri.equals(TEST_SOURCE_URI));
  do_check_true(items[0].target.file.equals(targetFile));
});




add_task(function test_load_string_malformed()
{
  let [list, store] = yield promiseNewListAndStore();

  let string = "[{\"source\":null,\"target\":null}," +
                "{\"source\":{\"uri\":\"about:blank\"}}";

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
