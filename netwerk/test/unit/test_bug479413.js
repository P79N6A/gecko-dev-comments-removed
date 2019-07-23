



const Cc = Components.classes;
const Ci = Components.interfaces;
var idnService;

function expected_pass(inputIDN)
{
  var isASCII = {};
  var displayIDN = idnService.convertToDisplayIDN(inputIDN, isASCII);
  do_check_eq(displayIDN, inputIDN);
}

function expected_fail(inputIDN)
{
  var isASCII = {};
  var displayIDN = "";

  try {
    displayIDN = idnService.convertToDisplayIDN(inputIDN, isASCII);
  }
  catch(e) {}

  do_check_neq(displayIDN, inputIDN);
}

function run_test() {
   
  var pbi = Cc["@mozilla.org/preferences-service;1"]
    .getService(Ci.nsIPrefBranch2);
  var whitelistPref = "network.IDN.whitelist.com";

  pbi.setBoolPref(whitelistPref, true);
 
  idnService = Cc["@mozilla.org/network/idn-service;1"]
    .getService(Ci.nsIIDNService);

  
  expected_pass("foo\u0101bar.com");

  
  
  expected_fail("xn--foobar-5za.com");

  
  expected_fail("foo\u3040bar.com");

  
  
  expected_pass("xn--foobar-533e.com");

  
  
  expected_fail("foo\u0370bar.com");

  
  if (pbi.prefHasUserValue(whitelistPref))
    pbi.clearUserPref(whitelistPref);
}
