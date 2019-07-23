







const STORAGE_TYPE = "mozStorage";

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

var expectedNotification;
var expectedData;

var TestObserver = {
  QueryInterface : XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsISupportsWeakReference]),

  observe : function (subject, topic, data) {
    do_check_eq(topic, "passwordmgr-storage-changed");
    do_check_eq(data, expectedNotification);

    switch (data) {
        case "addLogin":
            do_check_true(subject instanceof Ci.nsILoginInfo);
            do_check_true(subject instanceof Ci.nsILoginMetaInfo);
            do_check_true(expectedData.equals(subject)); 
            break;
        case "modifyLogin":
            do_check_true(subject instanceof Ci.nsIArray);
            do_check_eq(subject.length, 2);
            var oldLogin = subject.queryElementAt(0, Ci.nsILoginInfo);
            var newLogin = subject.queryElementAt(1, Ci.nsILoginInfo);
            do_check_true(expectedData[0].equals(oldLogin)); 
            do_check_true(expectedData[1].equals(newLogin));
            break;
        case "removeLogin":
            do_check_true(subject instanceof Ci.nsILoginInfo);
            do_check_true(subject instanceof Ci.nsILoginMetaInfo);
            do_check_true(expectedData.equals(subject)); 
            break;
        case "removeAllLogins":
            do_check_eq(subject, null);
            break;
        case "hostSavingEnabled":
        case "hostSavingDisabled":
            do_check_true(subject instanceof Ci.nsISupportsString);
            do_check_eq(subject.data, expectedData);
            break;
        default:
            do_throw("Unhandled notification: " + data + " / " + topic);
    }

    expectedNotification = null; 
    expectedData = null;
  }
};

function run_test() {

try {

var testnum = 0;
var testdesc = "Setup of nsLoginInfo test-users";
var nsLoginInfo = new Components.Constructor(
                    "@mozilla.org/login-manager/loginInfo;1",
                    Components.interfaces.nsILoginInfo);
do_check_true(nsLoginInfo != null);

var testuser1 = new nsLoginInfo;
testuser1.QueryInterface(Ci.nsILoginMetaInfo);
testuser1.init("http://testhost1", "", null,
    "dummydude", "itsasecret", "put_user_here", "put_pw_here");

var testuser2 = new nsLoginInfo;
testuser2.init("http://testhost2", "", null,
    "dummydude2", "itsasecret2", "put_user2_here", "put_pw2_here");





var os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
os.addObserver(TestObserver, "passwordmgr-storage-changed", false);



var testnum = 1;
var testdesc = "Initial connection to storage module"

LoginTest.deleteFile(OUTDIR, "signons-unittest-notify.sqlite");

var storage;
storage = LoginTest.initStorage(INDIR, "signons-empty.txt", OUTDIR, "signons-unittest-notify.sqlite");
var logins = storage.getAllLogins();
do_check_eq(logins.length, 0);
var disabledHosts = storage.getAllDisabledHosts();
do_check_eq(disabledHosts.length, 0, "Checking for no initial disabled hosts");


testnum++;
testdesc = "addLogin";

expectedNotification = "addLogin";
expectedData = testuser1;
storage.addLogin(testuser1);
LoginTest.checkStorageData(storage, [], [testuser1]);
do_check_eq(expectedNotification, null); 


testnum++;
testdesc = "modifyLogin";

expectedNotification = "modifyLogin";
expectedData=[testuser1, testuser2];
storage.modifyLogin(testuser1, testuser2);
do_check_eq(expectedNotification, null);
LoginTest.checkStorageData(storage, [], [testuser2]);


testnum++;
testdesc = "removeLogin";

expectedNotification = "removeLogin";
expectedData = testuser2;
storage.removeLogin(testuser2);
do_check_eq(expectedNotification, null);
LoginTest.checkStorageData(storage, [], []);


testnum++;
testdesc = "removeAllLogins";

expectedNotification = "removeAllLogins";
expectedData = null;
storage.removeAllLogins();
do_check_eq(expectedNotification, null);
LoginTest.checkStorageData(storage, [], []);


testnum++;
testdesc = "removeAllLogins (again)";

expectedNotification = "removeAllLogins";
expectedData = null;
storage.removeAllLogins();
do_check_eq(expectedNotification, null);
LoginTest.checkStorageData(storage, [], []);


testnum++;
testdesc = "setLoginSavingEnabled / false";

expectedNotification = "hostSavingDisabled";
expectedData = "http://site.com";
storage.setLoginSavingEnabled("http://site.com", false);
do_check_eq(expectedNotification, null);
LoginTest.checkStorageData(storage, ["http://site.com"], []);


testnum++;
testdesc = "setLoginSavingEnabled / false (again)";

expectedNotification = "hostSavingDisabled";
expectedData = "http://site.com";
storage.setLoginSavingEnabled("http://site.com", false);
do_check_eq(expectedNotification, null);
LoginTest.checkStorageData(storage, ["http://site.com"], []);


testnum++;
testdesc = "setLoginSavingEnabled / true";

expectedNotification = "hostSavingEnabled";
expectedData = "http://site.com";
storage.setLoginSavingEnabled("http://site.com", true);
do_check_eq(expectedNotification, null);
LoginTest.checkStorageData(storage, [], []);


testnum++;
testdesc = "setLoginSavingEnabled / true (again)";

expectedNotification = "hostSavingEnabled";
expectedData = "http://site.com";
storage.setLoginSavingEnabled("http://site.com", true);
do_check_eq(expectedNotification, null);
LoginTest.checkStorageData(storage, [], []);


LoginTest.deleteFile(OUTDIR, "signons-unittest-notify.sqlite");

} catch (e) {
    throw "FAILED in test #" + testnum + " -- " + testdesc + ": " + e;
}
};
