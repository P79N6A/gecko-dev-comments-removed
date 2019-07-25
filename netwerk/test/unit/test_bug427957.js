



var Cc = Components.classes;
var Ci = Components.interfaces;
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
    .getService(Ci.nsIPrefBranch);
  pbi.setBoolPref("network.IDN.whitelist.com", true);
 
  idnService = Cc["@mozilla.org/network/idn-service;1"]
    .getService(Ci.nsIIDNService);
  






  
  expected_fail("foo\u0340bar.com");
  
  expected_fail("foo\u0341bar.com");
  
  expected_fail("foo\200ebar.com");
  
  
  expected_fail("\u200f\u0645\u062B\u0627\u0644.\u0622\u0632\u0645\u0627\u06CC\u0634\u06CC");
  
  expected_fail("foo\u202abar.com");
  
  expected_fail("foo\u202bbar.com");
  
  expected_fail("foo\u202cbar.com");
  
  expected_fail("foo\u202dbar.com");
  
  expected_fail("foo\u202ebar.com");
  
  expected_fail("foo\u206abar.com");
  
  expected_fail("foo\u206bbar.com");
  
  expected_fail("foo\u206cbar.com");
  
  expected_fail("foo\u206dbar.com");
  
  expected_fail("foo\u206ebar.com");
  
  expected_fail("foo\u206fbar.com");

  


   

  
  expected_fail("www.\u05DE\u05D9\u05E5petel.com");
  
  
  expected_pass("www.\u05DE\u05D9\u05E5\u05E4\u05D8\u05DC.com");

  





  
  expected_fail("www.1\u05DE\u05D9\u05E5.com");
  
  expected_fail("www.\u05DE\u05D9\u05E51.com");
  
  expected_pass("www.\u05DE\u05D9\u05E51\u05E4\u05D8\u05DC.com");
}
  
