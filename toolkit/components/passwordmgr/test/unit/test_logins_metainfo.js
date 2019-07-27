









"use strict";




XPCOMUtils.defineLazyServiceGetter(this, "gUUIDGenerator",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

let gLooksLikeUUIDRegex = /^\{\w{8}-\w{4}-\w{4}-\w{4}-\w{12}\}$/;






function retrieveLoginMatching(aLoginInfo)
{
  let logins = Services.logins.findLogins({}, aLoginInfo.hostname, "", "");
  do_check_eq(logins.length, 1);
  return logins[0].QueryInterface(Ci.nsILoginMetaInfo);
}





function assertMetaInfoEqual(aActual, aExpected)
{
  do_check_neq(aActual, aExpected);

  
  do_check_true(aActual.equals(aExpected));

  
  do_check_eq(aActual.guid, aExpected.guid);
  do_check_eq(aActual.timeCreated, aExpected.timeCreated);
  do_check_eq(aActual.timeLastUsed, aExpected.timeLastUsed);
  do_check_eq(aActual.timePasswordChanged, aExpected.timePasswordChanged);
  do_check_eq(aActual.timesUsed, aExpected.timesUsed);
}




let gLoginInfo1;
let gLoginInfo2;
let gLoginInfo3;





let gLoginMetaInfo1;
let gLoginMetaInfo2;
let gLoginMetaInfo3;







add_task(function test_initialize()
{
  
  
  let baseTimeMs = Date.now() - 600000;

  gLoginInfo1 = TestData.formLogin();
  gLoginInfo2 = TestData.formLogin({
    hostname: "http://other.example.com",
    guid: gUUIDGenerator.generateUUID().toString(),
    timeCreated: baseTimeMs,
    timeLastUsed: baseTimeMs + 2,
    timePasswordChanged: baseTimeMs + 1,
    timesUsed: 2,
  });
  gLoginInfo3 = TestData.authLogin();
});





add_task(function test_addLogin_metainfo()
{
  
  Services.logins.addLogin(gLoginInfo1);

  
  do_check_eq(gLoginInfo1.guid, null);
  do_check_eq(gLoginInfo1.timeCreated, 0);
  do_check_eq(gLoginInfo1.timeLastUsed, 0);
  do_check_eq(gLoginInfo1.timePasswordChanged, 0);
  do_check_eq(gLoginInfo1.timesUsed, 0);

  
  gLoginMetaInfo1 = retrieveLoginMatching(gLoginInfo1);
  do_check_true(gLooksLikeUUIDRegex.test(gLoginMetaInfo1.guid));
  let creationTime = gLoginMetaInfo1.timeCreated;
  LoginTestUtils.assertTimeIsAboutNow(creationTime);
  do_check_eq(gLoginMetaInfo1.timeLastUsed, creationTime);
  do_check_eq(gLoginMetaInfo1.timePasswordChanged, creationTime);
  do_check_eq(gLoginMetaInfo1.timesUsed, 1);

  
  let originalLogin = gLoginInfo2.clone().QueryInterface(Ci.nsILoginMetaInfo);
  Services.logins.addLogin(gLoginInfo2);

  
  assertMetaInfoEqual(gLoginInfo2, originalLogin);

  
  gLoginMetaInfo2 = retrieveLoginMatching(gLoginInfo2);
  assertMetaInfoEqual(gLoginMetaInfo2, gLoginInfo2);

  
  Services.logins.addLogin(gLoginInfo3);
  gLoginMetaInfo3 = retrieveLoginMatching(gLoginInfo3);
  LoginTestUtils.checkLogins([gLoginInfo1, gLoginInfo2, gLoginInfo3]);
});




add_task(function test_addLogin_metainfo_duplicate()
{
  let loginInfo = TestData.formLogin({
    hostname: "http://duplicate.example.com",
    guid: gLoginMetaInfo2.guid,
  });
  Assert.throws(() => Services.logins.addLogin(loginInfo),
                /specified GUID already exists/);

  
  LoginTestUtils.checkLogins([gLoginInfo1, gLoginInfo2, gLoginInfo3]);
});





add_task(function test_modifyLogin_nsILoginInfo_metainfo_ignored()
{
  let newLoginInfo = gLoginInfo1.clone().QueryInterface(Ci.nsILoginMetaInfo);
  newLoginInfo.guid = gUUIDGenerator.generateUUID().toString();
  newLoginInfo.timeCreated = Date.now();
  newLoginInfo.timeLastUsed = Date.now();
  newLoginInfo.timePasswordChanged = Date.now();
  newLoginInfo.timesUsed = 12;
  Services.logins.modifyLogin(gLoginInfo1, newLoginInfo);

  newLoginInfo = retrieveLoginMatching(gLoginInfo1);
  assertMetaInfoEqual(newLoginInfo, gLoginMetaInfo1);
});




add_task(function test_modifyLogin_nsIProperyBag_metainfo()
{
  
  let newTimeMs = Date.now() + 120000;
  let newUUIDValue = gUUIDGenerator.generateUUID().toString();

  
  Services.logins.modifyLogin(gLoginInfo1, newPropertyBag({
    guid: newUUIDValue,
    timeCreated: newTimeMs,
    timeLastUsed: newTimeMs + 2,
    timePasswordChanged: newTimeMs + 1,
    timesUsed: 2,
  }));

  gLoginMetaInfo1 = retrieveLoginMatching(gLoginInfo1);
  do_check_eq(gLoginMetaInfo1.guid, newUUIDValue);
  do_check_eq(gLoginMetaInfo1.timeCreated, newTimeMs);
  do_check_eq(gLoginMetaInfo1.timeLastUsed, newTimeMs + 2);
  do_check_eq(gLoginMetaInfo1.timePasswordChanged, newTimeMs + 1);
  do_check_eq(gLoginMetaInfo1.timesUsed, 2);

  
  let originalLogin = gLoginInfo2.clone().QueryInterface(Ci.nsILoginMetaInfo);
  Services.logins.modifyLogin(gLoginInfo2, newPropertyBag({
    password: "new password",
  }));
  gLoginInfo2.password = "new password";

  gLoginMetaInfo2 = retrieveLoginMatching(gLoginInfo2);
  do_check_eq(gLoginMetaInfo2.password, gLoginInfo2.password);
  do_check_eq(gLoginMetaInfo2.timeCreated, originalLogin.timeCreated);
  do_check_eq(gLoginMetaInfo2.timeLastUsed, originalLogin.timeLastUsed);
  LoginTestUtils.assertTimeIsAboutNow(gLoginMetaInfo2.timePasswordChanged);

  
  
  Services.logins.modifyLogin(gLoginInfo2, newPropertyBag({
    password: "other password",
    timePasswordChanged: newTimeMs,
  }));
  gLoginInfo2.password = "other password";

  gLoginMetaInfo2 = retrieveLoginMatching(gLoginInfo2);
  do_check_eq(gLoginMetaInfo2.password, gLoginInfo2.password);
  do_check_eq(gLoginMetaInfo2.timeCreated, originalLogin.timeCreated);
  do_check_eq(gLoginMetaInfo2.timeLastUsed, originalLogin.timeLastUsed);
  do_check_eq(gLoginMetaInfo2.timePasswordChanged, newTimeMs);

  
  Services.logins.modifyLogin(gLoginInfo2, newPropertyBag({
    timesUsedIncrement: 2,
  }));

  gLoginMetaInfo2 = retrieveLoginMatching(gLoginInfo2);
  do_check_eq(gLoginMetaInfo2.timeCreated, originalLogin.timeCreated);
  do_check_eq(gLoginMetaInfo2.timeLastUsed, originalLogin.timeLastUsed);
  do_check_eq(gLoginMetaInfo2.timePasswordChanged, newTimeMs);
  do_check_eq(gLoginMetaInfo2.timesUsed, 4);
});




add_task(function test_modifyLogin_nsIProperyBag_metainfo_duplicate()
{
  Assert.throws(() => Services.logins.modifyLogin(gLoginInfo1, newPropertyBag({
    guid: gLoginInfo2.guid,
  })), /specified GUID already exists/);
  LoginTestUtils.checkLogins([gLoginInfo1, gLoginInfo2, gLoginInfo3]);
});




add_task(function test_searchLogins_metainfo()
{
  
  let logins = Services.logins.searchLogins({}, newPropertyBag({
    guid: gLoginMetaInfo1.guid,
  }));
  do_check_eq(logins.length, 1);
  let foundLogin = logins[0].QueryInterface(Ci.nsILoginMetaInfo);
  assertMetaInfoEqual(foundLogin, gLoginMetaInfo1);

  
  logins = Services.logins.searchLogins({}, newPropertyBag({
    timePasswordChanged: gLoginMetaInfo2.timePasswordChanged,
  }));
  do_check_eq(logins.length, 1);
  foundLogin = logins[0].QueryInterface(Ci.nsILoginMetaInfo);
  assertMetaInfoEqual(foundLogin, gLoginMetaInfo2);

  
  logins = Services.logins.searchLogins({}, newPropertyBag({
    guid: gLoginMetaInfo3.guid,
    timePasswordChanged: gLoginMetaInfo3.timePasswordChanged,
  }));
  do_check_eq(logins.length, 1);
  foundLogin = logins[0].QueryInterface(Ci.nsILoginMetaInfo);
  assertMetaInfoEqual(foundLogin, gLoginMetaInfo3);
});





add_task(function test_storage_metainfo()
{
  yield LoginTestUtils.reloadData();
  LoginTestUtils.checkLogins([gLoginInfo1, gLoginInfo2, gLoginInfo3]);

  assertMetaInfoEqual(retrieveLoginMatching(gLoginInfo1), gLoginMetaInfo1);
  assertMetaInfoEqual(retrieveLoginMatching(gLoginInfo2), gLoginMetaInfo2);
  assertMetaInfoEqual(retrieveLoginMatching(gLoginInfo3), gLoginMetaInfo3);
});
