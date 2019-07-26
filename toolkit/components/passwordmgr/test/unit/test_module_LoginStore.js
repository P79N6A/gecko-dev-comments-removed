








"use strict";




XPCOMUtils.defineLazyModuleGetter(this, "LoginStore",
                                  "resource://gre/modules/LoginStore.jsm");

const TEST_STORE_FILE_NAME = "test-logins.json";







add_task(function test_save_reload()
{
  let storeForSave = new LoginStore(getTempFile(TEST_STORE_FILE_NAME).path);

  
  yield storeForSave.load();

  let rawLoginData = {
    id:                  storeForSave.data.nextId++,
    hostname:            "http://www.example.com",
    httpRealm:           null,
    formSubmitURL:       "http://www.example.com/submit-url",
    usernameField:       "field_" + String.fromCharCode(533, 537, 7570, 345),
    passwordField:       "field_" + String.fromCharCode(421, 259, 349, 537),
    encryptedUsername:   "(test)",
    encryptedPassword:   "(test)",
    guid:                "(test)",
    encType:             Ci.nsILoginManagerCrypto.ENCTYPE_SDR,
    timeCreated:         Date.now(),
    timeLastUsed:        Date.now(),
    timePasswordChanged: Date.now(),
    timesUsed:           1,
  };
  storeForSave.data.logins.push(rawLoginData);

  storeForSave.data.disabledHosts.push("http://www.example.org");

  yield storeForSave.save();

  
  let storeForLoad = new LoginStore(storeForSave.path);
  yield storeForLoad.load();

  do_check_eq(storeForLoad.data.logins.length, 1);
  do_check_matches(storeForLoad.data.logins[0], rawLoginData);
  do_check_eq(storeForLoad.data.disabledHosts.length, 1);
  do_check_eq(storeForLoad.data.disabledHosts[0], "http://www.example.org");

  
  storeForLoad = new LoginStore(storeForSave.path);
  storeForLoad.ensureDataReady();

  do_check_eq(storeForLoad.data.logins.length, 1);
  do_check_matches(storeForLoad.data.logins[0], rawLoginData);
  do_check_eq(storeForLoad.data.disabledHosts.length, 1);
  do_check_eq(storeForLoad.data.disabledHosts[0], "http://www.example.org");
});




add_task(function test_load_empty()
{
  let store = new LoginStore(getTempFile(TEST_STORE_FILE_NAME).path);

  do_check_false(yield OS.File.exists(store.path));

  yield store.load();

  do_check_false(yield OS.File.exists(store.path));

  do_check_eq(store.data.logins.length, 0);
  do_check_eq(store.data.disabledHosts.length, 0);
});




add_task(function test_save_empty()
{
  let store = new LoginStore(getTempFile(TEST_STORE_FILE_NAME).path);

  yield store.load();

  let createdFile = yield OS.File.open(store.path, { create: true });
  yield createdFile.close();

  yield store.save();

  do_check_true(yield OS.File.exists(store.path));
});





add_task(function test_load_string_predefined()
{
  let store = new LoginStore(getTempFile(TEST_STORE_FILE_NAME).path);

  let string = "{\"logins\":[{" +
                "\"id\":1," +
                "\"hostname\":\"http://www.example.com\"," +
                "\"httpRealm\":null," +
                "\"formSubmitURL\":\"http://www.example.com/submit-url\"," +
                "\"usernameField\":\"usernameField\"," +
                "\"passwordField\":\"passwordField\"," +
                "\"encryptedUsername\":\"(test)\"," +
                "\"encryptedPassword\":\"(test)\"," +
                "\"guid\":\"(test)\"," +
                "\"encType\":1," +
                "\"timeCreated\":1262304000000," +
                "\"timeLastUsed\":1262390400000," +
                "\"timePasswordChanged\":1262476800000," +
                "\"timesUsed\":1}],\"disabledHosts\":[" +
                "\"http://www.example.org\"]}";

  yield OS.File.writeAtomic(store.path,
                            new TextEncoder().encode(string),
                            { tmpPath: store.path + ".tmp" });

  yield store.load();

  do_check_eq(store.data.logins.length, 1);
  do_check_matches(store.data.logins[0], {
    id:                  1,
    hostname:            "http://www.example.com",
    httpRealm:           null,
    formSubmitURL:       "http://www.example.com/submit-url",
    usernameField:       "usernameField",
    passwordField:       "passwordField",
    encryptedUsername:   "(test)",
    encryptedPassword:   "(test)",
    guid:                "(test)",
    encType:             Ci.nsILoginManagerCrypto.ENCTYPE_SDR,
    timeCreated:         1262304000000,
    timeLastUsed:        1262390400000,
    timePasswordChanged: 1262476800000,
    timesUsed:           1,
  });

  do_check_eq(store.data.disabledHosts.length, 1);
  do_check_eq(store.data.disabledHosts[0], "http://www.example.org");
});




add_task(function test_load_string_malformed()
{
  let store = new LoginStore(getTempFile(TEST_STORE_FILE_NAME).path);

  let string = "{\"logins\":[{\"hostname\":\"http://www.example.com\"," +
                "\"id\":1,";

  yield OS.File.writeAtomic(store.path, new TextEncoder().encode(string),
                            { tmpPath: store.path + ".tmp" });

  yield store.load();

  
  do_check_true(yield OS.File.exists(store.path + ".corrupt"));
  yield OS.File.remove(store.path + ".corrupt");

  
  do_check_eq(store.data.logins.length, 0);
  do_check_eq(store.data.disabledHosts.length, 0);
});





add_task(function test_load_string_malformed_sync()
{
  let store = new LoginStore(getTempFile(TEST_STORE_FILE_NAME).path);

  let string = "{\"logins\":[{\"hostname\":\"http://www.example.com\"," +
                "\"id\":1,";

  yield OS.File.writeAtomic(store.path, new TextEncoder().encode(string),
                            { tmpPath: store.path + ".tmp" });

  store.ensureDataReady();

  
  do_check_true(yield OS.File.exists(store.path + ".corrupt"));
  yield OS.File.remove(store.path + ".corrupt");

  
  do_check_eq(store.data.logins.length, 0);
  do_check_eq(store.data.disabledHosts.length, 0);
});
