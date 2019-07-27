












"use strict";












function buildExpectedLogins(aQuery)
{
  return TestData.loginList().filter(
    entry => Object.keys(aQuery).every(name => entry[name] === aQuery[name]));
}













function checkSearchLogins(aQuery, aExpectedCount)
{
  do_print("Testing searchLogins for " + JSON.stringify(aQuery));

  let expectedLogins = buildExpectedLogins(aQuery);
  do_check_eq(expectedLogins.length, aExpectedCount);

  let outCount = {};
  let logins = Services.logins.searchLogins(outCount, newPropertyBag(aQuery));
  do_check_eq(outCount.value, expectedLogins.length);
  LoginTestUtils.assertLoginListsEqual(logins, expectedLogins);
}














function checkAllSearches(aQuery, aExpectedCount)
{
  do_print("Testing all search functions for " + JSON.stringify(aQuery));

  let expectedLogins = buildExpectedLogins(aQuery);
  do_check_eq(expectedLogins.length, aExpectedCount);

  
  
  
  let hostname = ("hostname" in aQuery) ? aQuery.hostname : "";
  let formSubmitURL = ("formSubmitURL" in aQuery) ? aQuery.formSubmitURL : "";
  let httpRealm = ("httpRealm" in aQuery) ? aQuery.httpRealm : "";

  
  let outCount = {};
  let logins = Services.logins.findLogins(outCount, hostname, formSubmitURL,
                                          httpRealm);
  do_check_eq(outCount.value, expectedLogins.length);
  LoginTestUtils.assertLoginListsEqual(logins, expectedLogins);

  
  let count = Services.logins.countLogins(hostname, formSubmitURL, httpRealm)
  do_check_eq(count, expectedLogins.length);

  
  checkSearchLogins(aQuery, aExpectedCount);
}


















function checkAllSearchesTwoSets(aQuery, expectedQuery, aExpectedCount)
{
  do_print("Testing all search functions for " + JSON.stringify(aQuery) +
           " and " + JSON.stringify(expectedQuery));

  let expectedLogins = buildExpectedLogins(expectedQuery);

  
  
  
  let hostname = ("hostname" in aQuery) ? aQuery.hostname : "";
  let formSubmitURL = ("formSubmitURL" in aQuery) ? aQuery.formSubmitURL : "";
  let httpRealm = ("httpRealm" in aQuery) ? aQuery.httpRealm : "";

  
  let outCount = {};
  let logins = Services.logins.findLogins(outCount, hostname, formSubmitURL,
                                          httpRealm);
  do_check_eq(outCount.value, expectedLogins.length);
  LoginTestUtils.assertLoginListsMatches(logins, expectedLogins, true);

  
  let count = Services.logins.countLogins(hostname, formSubmitURL, httpRealm)
  do_check_eq(count, expectedLogins.length);

  
  outCount = {};
  logins = Services.logins.searchLogins(outCount, newPropertyBag(expectedQuery));
  do_check_eq(outCount.value, expectedLogins.length);
  LoginTestUtils.assertLoginListsMatches(logins, expectedLogins, true);
}







add_task(function test_initialize()
{
  for (let login of TestData.loginList()) {
    Services.logins.addLogin(login);
  }
});




add_task(function test_search_all_basic()
{
  
  checkAllSearches({}, 22);

  
  checkAllSearches({ httpRealm: null }, 13);
  checkAllSearches({ formSubmitURL: null }, 9);

  
  checkAllSearches({ hostname: "http://www4.example.com",
                     httpRealm: null }, 3);
  checkAllSearches({ hostname: "http://www2.example.org",
                     formSubmitURL: null }, 2);

  
  checkAllSearches({ hostname: "http://www.example.com" }, 1);
  checkAllSearches({ hostname: "https://www.example.com" }, 1);
  checkAllSearches({ hostname: "https://example.com" }, 1);
  checkAllSearches({ hostname: "http://www3.example.com" }, 2);

  
  checkAllSearches({ formSubmitURL: "http://www.example.com" }, 2);
  checkAllSearches({ formSubmitURL: "https://www.example.com" }, 1);
  checkAllSearches({ formSubmitURL: "http://example.com" }, 1);

  
  checkAllSearches({ hostname: "http://www3.example.com",
                     formSubmitURL: "http://www.example.com" }, 1);
  checkAllSearches({ hostname: "http://www3.example.com",
                     formSubmitURL: "http://example.com" }, 1);

  
  checkAllSearches({ httpRealm: "The HTTP Realm" }, 3);
  checkAllSearches({ httpRealm: "ftp://ftp.example.org" }, 1);
  checkAllSearches({ httpRealm: "The HTTP Realm Other" }, 2);

  
  checkAllSearches({ hostname: "http://example.net",
                     httpRealm: "The HTTP Realm" }, 1);
  checkAllSearches({ hostname: "http://example.net",
                     httpRealm: "The HTTP Realm Other" }, 1);
  checkAllSearches({ hostname: "ftp://example.net",
                     httpRealm: "ftp://example.net" }, 1);
});




add_task(function test_searchLogins()
{
  checkSearchLogins({ usernameField: "form_field_username" }, 11);
  checkSearchLogins({ passwordField: "form_field_password" }, 12);

  
  checkSearchLogins({ usernameField: "" }, 11);

  
  checkSearchLogins({ httpRealm: null,
                      usernameField: "" }, 2);

  
  checkSearchLogins({ hostname: "http://www6.example.com",
                      usernameField: "" }, 1);
});




add_task(function test_searchLogins_invalid()
{
  Assert.throws(() => Services.logins.searchLogins({},
                                      newPropertyBag({ username: "value" })),
                /Unexpected field/);
});





add_task(function test_search_all_full_case_sensitive()
{
  checkAllSearches({ hostname: "http://www.example.com" }, 1);
  checkAllSearches({ hostname: "http://www.example.com/" }, 0);
  checkAllSearches({ hostname: "http://" }, 0);
  checkAllSearches({ hostname: "example.com" }, 0);

  checkAllSearches({ formSubmitURL: "http://www.example.com" }, 2);
  checkAllSearches({ formSubmitURL: "http://" }, 0);
  Assert.throws(() => checkAllSearches({ formSubmitURL: "example.com" }, 0), /NS_ERROR_MALFORMED_URI/);

  checkAllSearches({ httpRealm: "The HTTP Realm" }, 3);
  checkAllSearches({ httpRealm: "The http Realm" }, 0);
  checkAllSearches({ httpRealm: "The HTTP" }, 0);
  checkAllSearches({ httpRealm: "Realm" }, 0);
});





add_task(function test_search_all_empty()
{
  checkAllSearches({ hostname: "http://nonexistent.example.com" }, 0);
  checkAllSearches({ formSubmitURL: "http://www.example.com",
                     httpRealm: "The HTTP Realm" }, 0);

  checkSearchLogins({ hostname: "" }, 0);
  checkSearchLogins({ id: "1000" }, 0);
});


add_task(function test_search_different_formSubmitURL_scheme()
{
  let aQuery = {
    formSubmitURL: "https://www.example.com",
    hostname: "http://www.example.com",
  };

  let buildQuery = {
    formSubmitURL: "http://www.example.com",
    hostname: "http://www.example.com",
  }

  checkAllSearchesTwoSets(aQuery, buildQuery, 1);
});
