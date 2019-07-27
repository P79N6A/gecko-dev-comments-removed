




let schema_version3 = `
PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
  CREATE TABLE groups (id INTEGER PRIMARY KEY, name TEXT NOT NULL);
  INSERT INTO "groups" VALUES(1,'foo.com');
  INSERT INTO "groups" VALUES(2,'bar.com');

  CREATE TABLE settings (id INTEGER PRIMARY KEY, name TEXT NOT NULL);
  INSERT INTO "settings" VALUES(1,'zoom-setting');
  INSERT INTO "settings" VALUES(2,'dir-setting');

  CREATE TABLE prefs (id INTEGER PRIMARY KEY, groupID INTEGER REFERENCES groups(id), settingID INTEGER NOT NULL REFERENCES settings(id), value BLOB);
  INSERT INTO "prefs" VALUES(1,1,1,0.5);
  INSERT INTO "prefs" VALUES(2,1,2,'/download/dir');
  INSERT INTO "prefs" VALUES(3,2,1,0.3);
  INSERT INTO "prefs" VALUES(4,NULL,1,0.1);

  CREATE INDEX groups_idx ON groups(name);
  CREATE INDEX settings_idx ON settings(name);
  CREATE INDEX prefs_idx ON prefs(groupID, settingID);
COMMIT;`;

function prepareVersion3Schema(callback) {
  var dirService = Cc["@mozilla.org/file/directory_service;1"].
                       getService(Ci.nsIProperties);

  var dbFile = dirService.get("ProfD", Ci.nsIFile);
  dbFile.append("content-prefs.sqlite");

  var dbService = Cc["@mozilla.org/storage/service;1"].
                  getService(Ci.mozIStorageService);
  ok(!dbFile.exists(), "Db should not exist yet.");

  var dbConnection = dbService.openDatabase(dbFile);
  equal(dbConnection.schemaVersion, 0);

  dbConnection.executeSimpleSQL(schema_version3);
  dbConnection.schemaVersion = 3;

  dbConnection.close();
}

function run_test() {
  prepareVersion3Schema();
  runAsyncTests(tests, true);
}





let tests = [
  function testMigration() {
    
    schemaVersionIs(4);
    let dbExpectedState = [
      [null, "zoom-setting", 0.1],
      ["bar.com", "zoom-setting", 0.3],
      ["foo.com", "zoom-setting", 0.5],
      ["foo.com", "dir-setting", "/download/dir"],
    ];
    yield dbOK(dbExpectedState);

    
    yield cps.removeAllDomainsSince(1000, null, makeCallback());
    yield dbOK(dbExpectedState);

    yield cps.removeAllDomainsSince(0, null, makeCallback());
    yield dbOK([[null, "zoom-setting", 0.1]]);

    
    const timestamp = 1234;
    yield setWithDate("a.com", "pref-name", "val", timestamp);
    let actualTimestamp = yield getDate("a.com", "pref-name");
    equal(actualTimestamp, timestamp);
  }
];
