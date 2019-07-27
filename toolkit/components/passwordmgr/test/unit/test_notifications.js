



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

add_task(function test_notifications()
{

try {

var testnum = 0;
var testdesc = "Setup of nsLoginInfo test-users";

var testuser1 = new LoginInfo("http://testhost1", "", null,
    "dummydude", "itsasecret", "put_user_here", "put_pw_here");

var testuser2 = new LoginInfo("http://testhost2", "", null,
    "dummydude2", "itsasecret2", "put_user2_here", "put_pw2_here");

Services.obs.addObserver(TestObserver, "passwordmgr-storage-changed", false);



var testnum = 1;
var testdesc = "Initial connection to storage module"


testnum++;
testdesc = "addLogin";

expectedNotification = "addLogin";
expectedData = testuser1;
Services.logins.addLogin(testuser1);
LoginTestUtils.checkLogins([testuser1]);
do_check_eq(expectedNotification, null); 


testnum++;
testdesc = "modifyLogin";

expectedNotification = "modifyLogin";
expectedData=[testuser1, testuser2];
Services.logins.modifyLogin(testuser1, testuser2);
do_check_eq(expectedNotification, null);
LoginTestUtils.checkLogins([testuser2]);


testnum++;
testdesc = "removeLogin";

expectedNotification = "removeLogin";
expectedData = testuser2;
Services.logins.removeLogin(testuser2);
do_check_eq(expectedNotification, null);
LoginTestUtils.checkLogins([]);


testnum++;
testdesc = "removeAllLogins";

expectedNotification = "removeAllLogins";
expectedData = null;
Services.logins.removeAllLogins();
do_check_eq(expectedNotification, null);
LoginTestUtils.checkLogins([]);


testnum++;
testdesc = "removeAllLogins (again)";

expectedNotification = "removeAllLogins";
expectedData = null;
Services.logins.removeAllLogins();
do_check_eq(expectedNotification, null);
LoginTestUtils.checkLogins([]);


testnum++;
testdesc = "setLoginSavingEnabled / false";

expectedNotification = "hostSavingDisabled";
expectedData = "http://site.com";
Services.logins.setLoginSavingEnabled("http://site.com", false);
do_check_eq(expectedNotification, null);
LoginTestUtils.assertDisabledHostsEqual(Services.logins.getAllDisabledHosts(),
                                        ["http://site.com"]);


testnum++;
testdesc = "setLoginSavingEnabled / false (again)";

expectedNotification = "hostSavingDisabled";
expectedData = "http://site.com";
Services.logins.setLoginSavingEnabled("http://site.com", false);
do_check_eq(expectedNotification, null);
LoginTestUtils.assertDisabledHostsEqual(Services.logins.getAllDisabledHosts(),
                                        ["http://site.com"]);


testnum++;
testdesc = "setLoginSavingEnabled / true";

expectedNotification = "hostSavingEnabled";
expectedData = "http://site.com";
Services.logins.setLoginSavingEnabled("http://site.com", true);
do_check_eq(expectedNotification, null);
LoginTestUtils.checkLogins([]);


testnum++;
testdesc = "setLoginSavingEnabled / true (again)";

expectedNotification = "hostSavingEnabled";
expectedData = "http://site.com";
Services.logins.setLoginSavingEnabled("http://site.com", true);
do_check_eq(expectedNotification, null);
LoginTestUtils.checkLogins([]);

Services.obs.removeObserver(TestObserver, "passwordmgr-storage-changed");

LoginTestUtils.clearData();

} catch (e) {
    throw new Error("FAILED in test #" + testnum + " -- " + testdesc + ": " + e);
}

});
