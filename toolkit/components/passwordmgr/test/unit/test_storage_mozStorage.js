





const ENCTYPE_BASE64 = 0;
const ENCTYPE_SDR = 1;



const CURRENT_SCHEMA = 5;

function copyFile(aLeafName)
{
  yield OS.File.copy(OS.Path.join(do_get_file("data").path, aLeafName),
                     OS.Path.join(OS.Constants.Path.profileDir, aLeafName));
};

function openDB(aLeafName)
{
  var dbFile = new FileUtils.File(OS.Constants.Path.profileDir);
  dbFile.append(aLeafName);

  return Services.storage.openDatabase(dbFile);
};

function deleteFile(pathname, filename)
{
  var file = new FileUtils.File(pathname);
  file.append(filename);

  
  
  
  try {
    if (file.exists())
      file.remove(false);
  } catch (e) {}
};

function reloadStorage(aInputPathName, aInputFileName)
{
  var inputFile = null;
  if (aInputFileName) {
      var inputFile  = Cc["@mozilla.org/file/local;1"].
                       createInstance(Ci.nsILocalFile);
      inputFile.initWithPath(aInputPathName);
      inputFile.append(aInputFileName);
  }

  let storage = Cc["@mozilla.org/login-manager/storage/mozStorage;1"]
                  .createInstance(Ci.nsILoginManagerStorage);
  storage.QueryInterface(Ci.nsIInterfaceRequestor)
         .getInterface(Ci.nsIVariant)
         .initWithFile(inputFile);

  return storage;
};

function checkStorageData(storage, ref_disabledHosts, ref_logins)
{
  LoginTestUtils.assertLoginListsEqual(storage.getAllLogins(), ref_logins);
  LoginTestUtils.assertDisabledHostsEqual(storage.getAllDisabledHosts(),
                                          ref_disabledHosts);
};

add_task(function test_execute()
{

const OUTDIR = OS.Constants.Path.profileDir;

try {

var isGUID = /^\{[0-9a-f\d]{8}-[0-9a-f\d]{4}-[0-9a-f\d]{4}-[0-9a-f\d]{4}-[0-9a-f\d]{12}\}$/;
function getGUIDforID(conn, id) {
    var stmt = conn.createStatement("SELECT guid from moz_logins WHERE id = " + id);
    stmt.executeStep();
    var guid = stmt.getString(0);
    stmt.finalize();
    return guid;
}

function getEncTypeForID(conn, id) {
    var stmt = conn.createStatement("SELECT encType from moz_logins WHERE id = " + id);
    stmt.executeStep();
    var encType = stmt.row.encType;
    stmt.finalize();
    return encType;
}

var storage;
var dbConnection;
var testnum = 0;
var testdesc = "Setup of nsLoginInfo test-users";
var nsLoginInfo = new Components.Constructor(
                    "@mozilla.org/login-manager/loginInfo;1",
                    Components.interfaces.nsILoginInfo);
do_check_true(nsLoginInfo != null);

var testuser1 = new nsLoginInfo;
testuser1.init("http://test.com", "http://test.com", null,
               "testuser1", "testpass1", "u1", "p1");
var testuser1B = new nsLoginInfo;
testuser1B.init("http://test.com", "http://test.com", null,
                "testuser1B", "testpass1B", "u1", "p1");
var testuser2 = new nsLoginInfo;
testuser2.init("http://test.org", "http://test.org", null,
               "testuser2", "testpass2", "u2", "p2");
var testuser3 = new nsLoginInfo;
testuser3.init("http://test.gov", "http://test.gov", null,
               "testuser3", "testpass3", "u3", "p3");
var testuser4 = new nsLoginInfo;
testuser4.init("http://test.gov", "http://test.gov", null,
               "testuser1", "testpass2", "u4", "p4");
var testuser5 = new nsLoginInfo;
testuser5.init("http://test.gov", "http://test.gov", null,
               "testuser2", "testpass1", "u5", "p5");



testnum++;
testdesc = "Test downgrade from v999 storage"

yield copyFile("signons-v999.sqlite");

dbConnection = openDB("signons-v999.sqlite");
do_check_eq(999, dbConnection.schemaVersion);
dbConnection.close();

storage = reloadStorage(OUTDIR, "signons-v999.sqlite");
checkStorageData(storage, ["https://disabled.net"], [testuser1]);


dbConnection = openDB("signons-v999.sqlite");
do_check_eq(CURRENT_SCHEMA, dbConnection.schemaVersion);
dbConnection.close();

deleteFile(OUTDIR, "signons-v999.sqlite");


testnum++;
testdesc = "Test downgrade from incompat v999 storage"


var origFile = OS.Path.join(OUTDIR, "signons-v999-2.sqlite");
var failFile = OS.Path.join(OUTDIR, "signons-v999-2.sqlite.corrupt");


yield copyFile("signons-v999-2.sqlite");
yield OS.File.remove(failFile);

Assert.throws(() => reloadStorage(OUTDIR, "signons-v999-2.sqlite"),
              /Initialization failed/);


do_check_false(yield OS.File.exists(origFile));
do_check_true(yield OS.File.exists(failFile));

yield OS.File.remove(failFile);


testnum++;
testdesc = "Test upgrade from v1->v2 storage"

yield copyFile("signons-v1.sqlite");

dbConnection = openDB("signons-v1.sqlite");
do_check_eq(1, dbConnection.schemaVersion);
dbConnection.close();

storage = reloadStorage(OUTDIR, "signons-v1.sqlite");
checkStorageData(storage, ["https://disabled.net"], [testuser1, testuser2]);


dbConnection = openDB("signons-v1.sqlite");
do_check_eq(CURRENT_SCHEMA, dbConnection.schemaVersion);
var guid = getGUIDforID(dbConnection, 1);
do_check_true(isGUID.test(guid));
guid = getGUIDforID(dbConnection, 2);
do_check_true(isGUID.test(guid));
dbConnection.close();

deleteFile(OUTDIR, "signons-v1.sqlite");


testnum++;
testdesc = "Test upgrade v2->v1 storage";




yield copyFile("signons-v1v2.sqlite");

dbConnection = openDB("signons-v1v2.sqlite");
do_check_eq(1, dbConnection.schemaVersion);
dbConnection.close();

storage = reloadStorage(OUTDIR, "signons-v1v2.sqlite");
checkStorageData(storage, ["https://disabled.net"], [testuser1, testuser2, testuser3]);



storage.modifyLogin(testuser1, testuser1B);
checkStorageData(storage, ["https://disabled.net"], [testuser1B, testuser2, testuser3]);



dbConnection = openDB("signons-v1v2.sqlite");
do_check_eq(CURRENT_SCHEMA, dbConnection.schemaVersion);
guid = getGUIDforID(dbConnection, 1);
do_check_eq("{655c7358-f1d6-6446-adab-53f98ac5d80f}", guid);
guid = getGUIDforID(dbConnection, 2);
do_check_eq("{13d9bfdc-572a-4d4e-9436-68e9803e84c1}", guid);
guid = getGUIDforID(dbConnection, 3);
do_check_true(isGUID.test(guid));
dbConnection.close();

deleteFile(OUTDIR, "signons-v1v2.sqlite");


testnum++;
testdesc = "Test upgrade from v2->v3 storage"

yield copyFile("signons-v2.sqlite");

dbConnection = openDB("signons-v2.sqlite");
do_check_eq(2, dbConnection.schemaVersion);

storage = reloadStorage(OUTDIR, "signons-v2.sqlite");


do_check_eq(CURRENT_SCHEMA, dbConnection.schemaVersion);
let encTypes = [ENCTYPE_BASE64, ENCTYPE_SDR, ENCTYPE_BASE64, ENCTYPE_BASE64];
for (let i = 0; i < encTypes.length; i++)
    do_check_eq(encTypes[i], getEncTypeForID(dbConnection, i + 1));
dbConnection.close();



checkStorageData(storage, ["https://disabled.net"],
    [testuser2]);

deleteFile(OUTDIR, "signons-v2.sqlite");


testnum++;
testdesc = "Test upgrade v3->v2 storage";




yield copyFile("signons-v2v3.sqlite");

dbConnection = openDB("signons-v2v3.sqlite");
do_check_eq(2, dbConnection.schemaVersion);
encTypes = [ENCTYPE_BASE64, ENCTYPE_SDR, ENCTYPE_BASE64, ENCTYPE_BASE64, null];
for (let i = 0; i < encTypes.length; i++)
    do_check_eq(encTypes[i], getEncTypeForID(dbConnection, i + 1));


storage = reloadStorage(OUTDIR, "signons-v2v3.sqlite");
do_check_eq(CURRENT_SCHEMA, dbConnection.schemaVersion);

encTypes = [ENCTYPE_BASE64, ENCTYPE_SDR, ENCTYPE_BASE64, ENCTYPE_BASE64, ENCTYPE_SDR];
for (let i = 0; i < encTypes.length; i++)
    do_check_eq(encTypes[i], getEncTypeForID(dbConnection, i + 1));




checkStorageData(storage, ["https://disabled.net"], [testuser2, testuser3]);
encTypes = [ENCTYPE_BASE64, ENCTYPE_SDR, ENCTYPE_BASE64, ENCTYPE_BASE64, ENCTYPE_SDR];
for (let i = 0; i < encTypes.length; i++)
    do_check_eq(encTypes[i], getEncTypeForID(dbConnection, i + 1));
dbConnection.close();

deleteFile(OUTDIR, "signons-v2v3.sqlite");



testnum++;
testdesc = "Test upgrade from v3->v4 storage"

yield copyFile("signons-v3.sqlite");

dbConnection = openDB("signons-v3.sqlite");
do_check_eq(3, dbConnection.schemaVersion);

storage = reloadStorage(OUTDIR, "signons-v3.sqlite");
do_check_eq(CURRENT_SCHEMA, dbConnection.schemaVersion);


checkStorageData(storage, [], [testuser1, testuser2]);

var logins = storage.getAllLogins();
for (var i = 0; i < 2; i++) {
    do_check_true(logins[i] instanceof Ci.nsILoginMetaInfo);
    do_check_eq(1, logins[i].timesUsed);
    LoginTestUtils.assertTimeIsAboutNow(logins[i].timeCreated);
    LoginTestUtils.assertTimeIsAboutNow(logins[i].timeLastUsed);
    LoginTestUtils.assertTimeIsAboutNow(logins[i].timePasswordChanged);
}


testnum++;
testdesc = "Test upgrade from v3->v4->v3 storage"

yield copyFile("signons-v3v4.sqlite");

dbConnection = openDB("signons-v3v4.sqlite");
do_check_eq(3, dbConnection.schemaVersion);

storage = reloadStorage(OUTDIR, "signons-v3v4.sqlite");
do_check_eq(CURRENT_SCHEMA, dbConnection.schemaVersion);


checkStorageData(storage, [], [testuser1, testuser2]);

var logins = storage.getAllLogins();

var t1, t2;
if (logins[0].username == "testuser1") {
    t1 = logins[0];
    t2 = logins[1];
} else {
    t1 = logins[1];
    t2 = logins[0];
}

do_check_true(t1 instanceof Ci.nsILoginMetaInfo);
do_check_true(t2 instanceof Ci.nsILoginMetaInfo);

do_check_eq(9, t1.timesUsed);
do_check_eq(1262049951275, t1.timeCreated);
do_check_eq(1262049951275, t1.timeLastUsed);
do_check_eq(1262049951275, t1.timePasswordChanged);

do_check_eq(1, t2.timesUsed);
LoginTestUtils.assertTimeIsAboutNow(t2.timeCreated);
LoginTestUtils.assertTimeIsAboutNow(t2.timeLastUsed);
LoginTestUtils.assertTimeIsAboutNow(t2.timePasswordChanged);



testnum++;
testdesc = "Test upgrade from v4 storage"

yield copyFile("signons-v4.sqlite");

dbConnection = openDB("signons-v4.sqlite");
do_check_eq(4, dbConnection.schemaVersion);
do_check_false(dbConnection.tableExists("moz_deleted_logins"));

storage = reloadStorage(OUTDIR, "signons-v4.sqlite");
do_check_eq(CURRENT_SCHEMA, dbConnection.schemaVersion);
do_check_true(dbConnection.tableExists("moz_deleted_logins"));



testnum++;
testdesc = "Test upgrade from v4->v5->v4 storage"

yield copyFile("signons-v4v5.sqlite");

dbConnection = openDB("signons-v4v5.sqlite");
do_check_eq(4, dbConnection.schemaVersion);
do_check_true(dbConnection.tableExists("moz_deleted_logins"));

storage = reloadStorage(OUTDIR, "signons-v4v5.sqlite");
do_check_eq(CURRENT_SCHEMA, dbConnection.schemaVersion);
do_check_true(dbConnection.tableExists("moz_deleted_logins"));



testnum++;
testdesc = "Create nsILoginInfo instances for testing with"

testuser1 = new nsLoginInfo;
testuser1.init("http://dummyhost.mozilla.org", "", null,
    "dummydude", "itsasecret", "put_user_here", "put_pw_here");









testnum++;
testdesc = "Corrupt database and backup"

const filename = "signons-c.sqlite";
const filepath = OS.Path.join(OS.Constants.Path.profileDir, filename);

yield OS.File.copy(do_get_file("data/corruptDB.sqlite").path, filepath);


Assert.throws(
  () => reloadStorage(OS.Constants.Path.profileDir, filename),
  /Initialization failed/);


do_check_true(yield OS.File.exists(filepath + ".corrupt"));


do_check_false(yield OS.File.exists(filepath));


storage = reloadStorage(OS.Constants.Path.profileDir, filename);


storage.addLogin(testuser1);
checkStorageData(storage, [], [testuser1]);


var file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
file.initWithPath(OS.Constants.Path.profileDir);
file.append(filename);
do_check_true(file.exists());

deleteFile(OS.Constants.Path.profileDir, filename + ".corrupt");
deleteFile(OS.Constants.Path.profileDir, filename);

} catch (e) {
    throw new Error("FAILED in test #" + testnum + " -- " + testdesc + ": " + e);
}

});
