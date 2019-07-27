


function test_locale() {
  
  Services.prefs.setCharPref("general.useragent.locale", "ab-CD");

  Assert.equal(MozLoopService.locale, "ab-CD");

  Services.prefs.clearUserPref("general.useragent.locale");
}

function test_getStrings() {
  
  Assert.equal(MozLoopService.getStrings("invalid_not_found_string"), "");

  
  
  Assert.equal(MozLoopService.getStrings("share_link_header_text"),
               '{"textContent":"Share this link to invite someone to talk:"}');
}

function run_test()
{
  setupFakeLoopServer();

  test_locale();
  test_getStrings();
}
