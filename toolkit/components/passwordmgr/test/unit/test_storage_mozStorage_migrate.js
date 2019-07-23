









const STORAGE_TYPE = "mozStorage";



const CURRENT_SCHEMA = 2;

function run_test() {

try {

var isGUID = /^\{[0-9a-f\d]{8}-[0-9a-f\d]{4}-[0-9a-f\d]{4}-[0-9a-f\d]{4}-[0-9a-f\d]{12}\}$/;
function getGUIDforID(conn, id) {
    var stmt = conn.createStatement("SELECT guid from moz_logins WHERE id = " + id);
    stmt.executeStep();
    var guid = stmt.getString(0);
    stmt.finalize();
    return guid;
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


testnum++;
testdesc = "Test downgrade from v999 storage"

LoginTest.copyFile("signons-v999.sqlite");

dbConnection = LoginTest.openDB("signons-v999.sqlite");
do_check_eq(999, dbConnection.schemaVersion);
dbConnection.close();

storage = LoginTest.reloadStorage(OUTDIR, "signons-v999.sqlite");
LoginTest.checkStorageData(storage, ["https://disabled.net"], [testuser1]);


dbConnection = LoginTest.openDB("signons-v999.sqlite");
do_check_eq(CURRENT_SCHEMA, dbConnection.schemaVersion);
dbConnection.close();

LoginTest.deleteFile(OUTDIR, "signons-v999.sqlite");


testnum++;
testdesc = "Test downgrade from incompat v999 storage"


var origFile = PROFDIR.clone();
origFile.append("signons-v999-2.sqlite");
var failFile = PROFDIR.clone();
failFile.append("signons-v999-2.sqlite.corrupt");


LoginTest.deleteFile(OUTDIR, "signons-v999-2.sqlite");
LoginTest.deleteFile(OUTDIR, "signons-v999-2.sqlite.corrupt");
do_check_false(origFile.exists());
do_check_false(failFile.exists());

LoginTest.copyFile("signons-v999-2.sqlite");
do_check_true(origFile.exists());

storage = LoginTest.reloadStorage(OUTDIR, "signons-v999-2.sqlite", /Initialization failed/);


do_check_false(origFile.exists());
do_check_true(failFile.exists());

LoginTest.deleteFile(OUTDIR, "signons-v999-2.sqlite");
LoginTest.deleteFile(OUTDIR, "signons-v999-2.sqlite.corrupt");


testnum++;
testdesc = "Test upgrade from v1 storage"

LoginTest.copyFile("signons-v1.sqlite");

dbConnection = LoginTest.openDB("signons-v1.sqlite");
do_check_eq(1, dbConnection.schemaVersion);
dbConnection.close();

storage = LoginTest.reloadStorage(OUTDIR, "signons-v1.sqlite");
LoginTest.checkStorageData(storage, ["https://disabled.net"], [testuser1, testuser2]);


dbConnection = LoginTest.openDB("signons-v1.sqlite");
do_check_eq(CURRENT_SCHEMA, dbConnection.schemaVersion);
var guid = getGUIDforID(dbConnection, 1);
do_check_true(isGUID.test(guid));
guid = getGUIDforID(dbConnection, 2);
do_check_true(isGUID.test(guid));
dbConnection.close();

LoginTest.deleteFile(OUTDIR, "signons-v1.sqlite");


testnum++;
testdesc = "Test upgrade v2->v1 storage";




LoginTest.copyFile("signons-v1v2.sqlite");

dbConnection = LoginTest.openDB("signons-v1v2.sqlite");
do_check_eq(1, dbConnection.schemaVersion);
dbConnection.close();

storage = LoginTest.reloadStorage(OUTDIR, "signons-v1v2.sqlite");
LoginTest.checkStorageData(storage, ["https://disabled.net"], [testuser1, testuser2, testuser3]);



storage.modifyLogin(testuser1, testuser1B);
LoginTest.checkStorageData(storage, ["https://disabled.net"], [testuser1B, testuser2, testuser3]);



dbConnection = LoginTest.openDB("signons-v1v2.sqlite");
do_check_eq(CURRENT_SCHEMA, dbConnection.schemaVersion);
guid = getGUIDforID(dbConnection, 1);
do_check_eq("{655c7358-f1d6-6446-adab-53f98ac5d80f}", guid);
guid = getGUIDforID(dbConnection, 2);
do_check_eq("{13d9bfdc-572a-4d4e-9436-68e9803e84c1}", guid);
guid = getGUIDforID(dbConnection, 3);
do_check_true(isGUID.test(guid));
dbConnection.close();

LoginTest.deleteFile(OUTDIR, "signons-v1v2.sqlite");


} catch (e) {
    throw "FAILED in test #" + testnum + " -- " + testdesc + ": " + e;
}
};
