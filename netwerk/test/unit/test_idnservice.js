

var reference = [
                 
                 
                 ["asciihost", "asciihost", false],
                 ["b\u00FCcher", "xn--bcher-kva", true]
                ];

function run_test() {
  var idnService = Components.classes["@mozilla.org/network/idn-service;1"]
                             .getService(Components.interfaces.nsIIDNService);

  for (var i = 0; i < reference.length; ++i) {
     dump("Testing " + reference[i] + "\n");
     
     
     
     
     do_check_eq(idnService.convertUTF8toACE(reference[i][0]), reference[i][1]);
     do_check_eq(idnService.convertUTF8toACE(reference[i][1]), reference[i][1]);
     do_check_eq(idnService.convertACEtoUTF8(reference[i][1]), reference[i][0]);
     do_check_eq(idnService.isACE(reference[i][1]), reference[i][2]);
  }

  
  var pbi = Components.classes["@mozilla.org/preferences-service;1"]
                      .getService(Components.interfaces.nsIPrefBranch2);
  pbi.setBoolPref("network.IDN.whitelist.es", true);

  
  var isASCII = {};
  do_check_eq(idnService.convertToDisplayIDN("b\u00FCcher.es", isASCII), "b\u00FCcher.es");
  do_check_eq(isASCII.value, false);
  do_check_eq(idnService.convertToDisplayIDN("xn--bcher-kva.es", isASCII), "b\u00FCcher.es");
  do_check_eq(isASCII.value, false);
  do_check_eq(idnService.convertToDisplayIDN("b\u00FCcher.uk", isASCII), "xn--bcher-kva.uk");
  do_check_eq(isASCII.value, true);
  do_check_eq(idnService.convertToDisplayIDN("xn--bcher-kva.uk", isASCII), "xn--bcher-kva.uk");
  do_check_eq(isASCII.value, true);
}
