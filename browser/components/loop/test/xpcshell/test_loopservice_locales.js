


const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "MozLoopService",
                                  "resource:///modules/loop/MozLoopService.jsm");


function test_locale() {
  
  Services.prefs.setCharPref("general.useragent.locale", "ab-CD");

  Assert.equal(MozLoopService.locale, "ab-CD");

  Services.prefs.clearUserPref("general.useragent.locale");
}

function test_getStrings() {
  
  Assert.equal(MozLoopService.getStrings("invalid_not_found_string"), "");

  
  
  
  Assert.equal(MozLoopService.getStrings("caller"), '{"placeholder":"Identify this call"}');
}

function run_test()
{
  test_locale();
  test_getStrings();
}
