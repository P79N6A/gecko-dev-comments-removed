

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
}
