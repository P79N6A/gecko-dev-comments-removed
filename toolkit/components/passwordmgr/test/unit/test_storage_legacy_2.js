








var DIR_SERVICE = new Components.Constructor(
                    "@mozilla.org/file/directory_service;1", "nsIProperties");
var TESTDIR = (new DIR_SERVICE()).get("ProfD", Ci.nsIFile).path;

var REFDIR = TESTDIR;

function areFilesIdentical(pathname1, filename1, pathname2, filename2) {
        var file1 = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
        var file2 = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
        file1.initWithPath(pathname1);
        file2.initWithPath(pathname2);
        file1.append(filename1);
        file2.append(filename2);
        if (!file1.exists()) { throw("areFilesIdentical: file " + filename1 + " does not exist."); }
        if (!file2.exists()) { throw("areFilesIdentical: file " + filename2 + " does not exist."); }

        var inputStream1 = Cc["@mozilla.org/network/file-input-stream;1"].createInstance(Ci.nsIFileInputStream);
        var inputStream2 = Cc["@mozilla.org/network/file-input-stream;1"].createInstance(Ci.nsIFileInputStream);
        inputStream1.init(file1, 0x01, 0600, null); 
        inputStream2.init(file2, 0x01, 0600, null); 

	const PR_UINT32_MAX = 0xffffffff; 
	var md5 = Cc["@mozilla.org/security/hash;1"].createInstance(Ci.nsICryptoHash);

	md5.init(md5.MD5);
	md5.updateFromStream(inputStream1, PR_UINT32_MAX);
	var hash1 = md5.finish(true);

	md5.init(md5.MD5);
	md5.updateFromStream(inputStream2, PR_UINT32_MAX);
	var hash2 = md5.finish(true);

	inputStream1.close();
	inputStream2.close();

	var identical = (hash1 == hash2);

	return identical;
}


function initStorage(inputfile, outputfile, path, storage, expectedError) {
	var e, caughtError = false;

	try {
		storage.initWithFile(path, inputfile, outputfile);
	} catch (e) { caughtError = true; var err = e;}

	if (expectedError) {
		if (!caughtError) { throw "Component should have returned an error (" + expectedError + "), but did not."; }

		if (!expectedError.test(err)) { throw "Expected error was of wrong type: expected (" + expectedError + "), got " + err; }

		
		dump("...that error was expected.\n\n");
	} else if (caughtError) {
		throw "Component threw unexpected error: " + err;
	}

	return;
};



function checkStorageData(storage, ref_disabledHosts, ref_logins) {

	var stor_disabledHosts = storage.getAllDisabledHosts({});
	do_check_eq(ref_disabledHosts.length, stor_disabledHosts.length);
	

	var stor_logins = storage.getAllLogins({});
	do_check_eq(ref_logins.length, stor_logins.length);

	



	var i, j, found;
	for (i = 0; i < ref_disabledHosts.length; i++) {
		for (j = 0; !found && j < stor_disabledHosts.length; j++) {
			found = (ref_disabledHosts[i] == stor_disabledHosts[j]);
		}
		do_check_true(found || stor_disabledHosts.length == 0);
	}
	for (j = 0; j < stor_disabledHosts.length; j++) {
		for (i = 0; !found && i < ref_disabledHosts.length; i++) {
			found = (ref_disabledHosts[i] == stor_disabledHosts[j]);
		}
		do_check_true(found || stor_disabledHosts.length == 0);
	}

	



	var ref, stor;
	for (i = 0; i < ref_logins.length; i++) {
		for (j = 0; !found && j < stor_logins.length; j++) {
			found = ref_logins[i].equals(stor_logins[j]);
		}
		do_check_true(found || stor_logins.length == 0);
	}

}


function run_test() {



return;

try {



var testnum = 0;
var testdesc = "Initial connection to storage module"

var storage = Cc["@mozilla.org/login-manager/storage/legacy;1"].createInstance(Ci.nsILoginManagerStorage);
if (!storage) throw "Couldn't create storage instance.";



testnum++;
var testdesc = "Create nsILoginInfo instances for testing with"

var dummyuser1 = Cc["@mozilla.org/login-manager/loginInfo;1"].createInstance(Ci.nsILoginInfo);
var dummyuser2 = Cc["@mozilla.org/login-manager/loginInfo;1"].createInstance(Ci.nsILoginInfo);
var dummyuser3 = Cc["@mozilla.org/login-manager/loginInfo;1"].createInstance(Ci.nsILoginInfo);

dummyuser1.hostname      = "http://dummyhost.mozilla.org";
dummyuser1.username      = "dummydude";
dummyuser1.usernameField = "put_user_here";
dummyuser1.password      = "itsasecret";
dummyuser1.passwordField = "put_pw_here";
dummyuser1.formSubmitURL = "";
dummyuser1.httpRealm     = null;

dummyuser2.hostname      = "http://dummyhost2.mozilla.org";
dummyuser2.username      = "dummydude2";
dummyuser2.usernameField = "put_user2_here";
dummyuser2.password      = "itsasecret2";
dummyuser2.passwordField = "put_pw2_here";
dummyuser2.formSubmitURL = "http://cgi.site.com";
dummyuser2.httpRealm     = null;

dummyuser3.hostname      = "http://dummyhost2.mozilla.org";
dummyuser3.username      = "dummydude3";
dummyuser3.usernameField = "put_user3_here";
dummyuser3.password      = "itsasecret3";
dummyuser3.passwordField = "put_pw3_here";
dummyuser3.formSubmitURL = "http://cgi.site.com";
dummyuser3.httpRealm     = null;

var logins, disabledHosts;


testnum++;

testdesc = "[ensuring file doesn't exist]";
var filename="non-existant-file-"+Math.floor(Math.random() * 10000);
var file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
file.initWithPath(TESTDIR);
file.append(filename);
var exists = file.exists();
if (exists) {
	
	file.remove(false);
	do_check_false(exists);
}

testdesc = "Initialize with no existing file";
initStorage(filename, null, TESTDIR, storage, null);

checkStorageData(storage, [], []);

testdesc = "Add 1 disabled host only";
storage.setLoginSavingEnabled("http://www.nsa.gov", false);
disabledHosts = ["http://www.nsa.gov"];
checkStorageData(storage, disabledHosts, []);


testdesc = "[moving file and checking contents]";
file.moveTo(null, "output-00.txt");
do_check_true(areFilesIdentical(TESTDIR, "output-00.txt", REFDIR, "reference-output-00.txt"));



testnum++;

testdesc = "[copying empty file to testing name]";
file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
file.initWithPath(REFDIR);
file.append("signons-empty.txt");
var file2 = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
file2.initWithPath(TESTDIR);
file2.append("output-01.txt");


if (file2.exists()) { file2.remove(false); }


do_check_true(file.exists());
file.copyTo(null, "output-01.txt");
do_check_true(areFilesIdentical(TESTDIR, "output-01.txt", REFDIR, "reference-empty.txt"));

testdesc = "Initialize with existing file (valid, but empty)";
initStorage("output-01.txt", null, TESTDIR, storage, null);
checkStorageData(storage, [], []);

testdesc = "Add 1 disabled host only";
storage.setLoginSavingEnabled("http://www.nsa.gov", false);

disabledHosts = ["http://www.nsa.gov"];
checkStorageData(storage, disabledHosts, []);
do_check_true(areFilesIdentical(TESTDIR, "output-01.txt", REFDIR, "reference-output-01a.txt"));

testdesc = "Remove disabled host only";
storage.setLoginSavingEnabled("http://www.nsa.gov", true);

checkStorageData(storage, [], []);
do_check_true(areFilesIdentical(TESTDIR, "output-01.txt", REFDIR, "reference-output-01b.txt"));



testnum++;

testdesc = "[clear data and reinitialize with signons-empty.txt]";
initStorage("signons-empty.txt", "output-02.txt", TESTDIR, storage, null);



testdesc = "Add 1 login only";
storage.addLogin(dummyuser1);

testdesc = "[flush and reload for verification]";
initStorage("output-02.txt", null, TESTDIR, storage, null);

testdesc = "Verify output-02.txt";
checkStorageData(storage, [], [dummyuser1]);




do_check_false(areFilesIdentical(TESTDIR, "output-02.txt", REFDIR, "reference-output-02.txt"));



testnum++;

testdesc = "[clear data and reinitialize with signons-empty.txt]";
initStorage("signons-empty.txt", "output-03.txt", TESTDIR, storage, null);

testdesc = "Add 1 disabled host only";
storage.setLoginSavingEnabled("http://www.nsa.gov", false);

testdesc = "[flush and reload for verification]";
initStorage("output-03.txt", null, TESTDIR, storage, null);

testdesc = "Verify output-03.txt";
checkStorageData(storage, ["http://www.nsa.gov"], []);



testnum++;

testdesc = "[clear data and reinitialize with signons-empty.txt]";
initStorage("signons-empty.txt", "output-04.txt", TESTDIR, storage, null);

testdesc = "Add 1 disabled host and 1 login";
storage.setLoginSavingEnabled("http://www.nsa.gov", false);
storage.addLogin(dummyuser1);

testdesc = "[flush and reload for verification]";
initStorage("output-04.txt", null, TESTDIR, storage, null);

testdesc = "Verify output-04.txt";
checkStorageData(storage, ["http://www.nsa.gov"], [dummyuser1]);



testnum++;

testdesc = "[clear data and reinitialize with signons-empty.txt]";
initStorage("signons-empty.txt", "output-03.txt", TESTDIR, storage, null);

testdesc = "Add 2 logins (to different hosts)";
storage.addLogin(dummyuser1);
storage.addLogin(dummyuser2);

testdesc = "[flush and reload for verification]";
initStorage("output-03.txt", null, TESTDIR, storage, null);

testdesc = "Verify output-03.txt";
checkStorageData(storage, [], [dummyuser2, dummyuser1]);



testnum++;

testdesc = "[clear data and reinitialize with signons-empty.txt]";
initStorage("signons-empty.txt", "output-04.txt", TESTDIR, storage, null);

testdesc = "Add 2 logins (to same host)";
storage.addLogin(dummyuser2);
storage.addLogin(dummyuser3);

testdesc = "[flush and reload for verification]";
initStorage("output-04.txt", null, TESTDIR, storage, null);

testdesc = "Verify output-04.txt";
logins = [dummyuser3, dummyuser2];
checkStorageData(storage, [], logins);



testnum++;

testdesc = "[clear data and reinitialize with signons-empty.txt]";
initStorage("signons-empty.txt", "output-05.txt", TESTDIR, storage, null);

testdesc = "Add 3 logins (2 to same host)";
storage.addLogin(dummyuser3);
storage.addLogin(dummyuser1);
storage.addLogin(dummyuser2);

testdesc = "[flush and reload for verification]";
initStorage("output-05.txt", null, TESTDIR, storage, null);

testdesc = "Verify output-05.txt";
logins = [dummyuser1, dummyuser2, dummyuser3];
checkStorageData(storage, [], logins);


testnum++;

testdesc = "Final sanity test to make sure signons-empty.txt wasn't modified.";
do_check_true(areFilesIdentical(TESTDIR, "signons-empty.txt", REFDIR, "reference-empty.txt"));









} catch (e) { throw ("FAILED in test #" + testnum + " -- " + testdesc + ": " + e); }
};
