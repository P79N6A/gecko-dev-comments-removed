


function test_locale() {
  
  Services.prefs.setCharPref("general.useragent.locale", "ab-CD");

  Assert.equal(MozLoopService.locale, "ab-CD");

  Services.prefs.clearUserPref("general.useragent.locale");
}

function test_getStrings() {
  
  Assert.equal(MozLoopService.getStrings("invalid_not_found_string"), "");

  
  
  Assert.equal(MozLoopService.getStrings("get_link_to_share"),
               '{"textContent":"Get a link and invite someone to talk"}');
}

function run_test()
{
  test_locale();
  test_getStrings();
}
