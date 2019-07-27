


XPCOMUtils.defineLazyModuleGetter(this, "PlacesTestUtils",
                                  "resource://testing-common/PlacesTestUtils.jsm");

let PERMISSIONS_FILE_NAME = "permissions.sqlite";

function GetPermissionsFile(profile)
{
  let file = profile.clone();
  file.append(PERMISSIONS_FILE_NAME);
  return file;
}

function run_test() {
  run_next_test();
}

add_task(function test() {
  
  let profile = do_get_profile();

  let db = Services.storage.openDatabase(GetPermissionsFile(profile));
  db.schemaVersion = 4;

  db.executeSimpleSQL(
    "CREATE TABLE moz_hosts (" +
      " id INTEGER PRIMARY KEY" +
      ",host TEXT" +
      ",type TEXT" +
      ",permission INTEGER" +
      ",expireType INTEGER" +
      ",expireTime INTEGER" +
      ",modificationTime INTEGER" +
      ",appId INTEGER" +
      ",isInBrowserElement INTEGER" +
    ")");

  let stmtInsert = db.createStatement(
    "INSERT INTO moz_hosts (" +
      "id, host, type, permission, expireType, expireTime, modificationTime, appId, isInBrowserElement" +
    ") VALUES (" +
      ":id, :host, :type, :permission, :expireType, :expireTime, :modificationTime, :appId, :isInBrowserElement" +
    ")");

  let id = 0;

  function insertHost(host, type, permission, expireType, expireTime, modificationTime, appId, isInBrowserElement) {
    stmtInsert.bindByName("id", id++);
    stmtInsert.bindByName("host", host);
    stmtInsert.bindByName("type", type);
    stmtInsert.bindByName("permission", permission);
    stmtInsert.bindByName("expireType", expireType);
    stmtInsert.bindByName("expireTime", expireTime);
    stmtInsert.bindByName("modificationTime", modificationTime);
    stmtInsert.bindByName("appId", appId);
    stmtInsert.bindByName("isInBrowserElement", isInBrowserElement);

    try {
      stmtInsert.executeStep();
      stmtInsert.reset();
    } catch (e) {
      stmtInsert.reset();
      throw e;
    }
  }

  
  insertHost("foo.com", "A", 1, 0, 0, 0, 0, false);
  insertHost("foo.com", "A", 1, 0, 0, 0, 1000, false);
  insertHost("foo.com", "A", 1, 0, 0, 0, 2000, true);
  insertHost("sub.foo.com", "B", 1, 0, 0, 0, 0, false);
  insertHost("subber.sub.foo.com", "B", 1, 0, 0, 0, 0, false);
  insertHost("bar.ca", "B", 1, 0, 0, 0, 0, false);
  insertHost("bar.ca", "B", 1, 0, 0, 0, 1000, false);
  insertHost("bar.ca", "A", 1, 0, 0, 0, 1000, true);
  insertHost("file:///some/path/to/file.html", "A", 1, 0, 0, 0, 0, false);
  insertHost("file:///another/file.html", "A", 1, 0, 0, 0, 0, false);
  insertHost("moz-nullprincipal:{8695105a-adbe-4e4e-8083-851faa5ca2d7}", "A", 1, 0, 0, 0, 0, false);
  insertHost("moz-nullprincipal:{12ahjksd-akjs-asd3-8393-asdu2189asdu}", "B", 1, 0, 0, 0, 0, false);
  insertHost("<file>", "A", 1, 0, 0, 0, 0, false);
  insertHost("<file>", "B", 1, 0, 0, 0, 0, false);

  
  stmtInsert.finalize();
  db.close();
  stmtInsert = null;
  db = null;

  let expected = [
    
    
    
    
    
    
    

    ["http://sub.foo.com", "B", 1, 0, 0],
    ["http://subber.sub.foo.com", "B", 1, 0, 0],

    ["https://foo.com", "A", 1, 0, 0],
    ["https://foo.com^appId=1000", "A", 1, 0, 0],
    ["https://foo.com^appId=2000&inBrowser=1", "A", 1, 0, 0],
    ["https://sub.foo.com", "B", 1, 0, 0],
    ["https://subber.sub.foo.com", "B", 1, 0, 0],

    
    ["http://bar.ca", "B", 1, 0, 0],
    ["https://bar.ca", "B", 1, 0, 0],
    ["http://bar.ca^appId=1000", "B", 1, 0, 0],
    ["https://bar.ca^appId=1000", "B", 1, 0, 0],
    ["http://bar.ca^appId=1000&inBrowser=1", "A", 1, 0, 0],
    ["https://bar.ca^appId=1000&inBrowser=1", "A", 1, 0, 0],
    ["file:///some/path/to/file.html", "A", 1, 0, 0],
    ["file:///another/file.html", "A", 1, 0, 0],

    
    
    ["ftp://foo.com:8000", "A", 1, 0, 0],
    ["ftp://foo.com:8000^appId=1000", "A", 1, 0, 0],
    ["ftp://foo.com:8000^appId=2000&inBrowser=1", "A", 1, 0, 0],
  ];

  let found = expected.map((it) => 0);

  
  yield PlacesTestUtils.addVisits(Services.io.newURI("https://foo.com/some/other/subdirectory", null, null));
  yield PlacesTestUtils.addVisits(Services.io.newURI("ftp://some.subdomain.of.foo.com:8000/some/subdirectory", null, null));

  
  let enumerator = Services.perms.enumerator;
  while (enumerator.hasMoreElements()) {
    let permission = enumerator.getNext().QueryInterface(Ci.nsIPermission);
    let isExpected = false;

    expected.forEach((it, i) => {
      if (permission.principal.origin == it[0] &&
          permission.type == it[1] &&
          permission.capability == it[2] &&
          permission.expireType == it[3] &&
          permission.expireTime == it[4]) {
        isExpected = true;
        found[i]++;
      }
    });

    do_check_true(isExpected,
                  "Permission " + (isExpected ? "should" : "shouldn't") +
                  " be in permission database: " +
                  permission.principal.origin + ", " +
                  permission.type + ", " +
                  permission.capability + ", " +
                  permission.expireType + ", " +
                  permission.expireTime);
  }

  found.forEach((count, i) => {
    do_check_true(count == 1, "Expected count = 1, got count = " + count + " for permission " + expected[i]);
  });
});
