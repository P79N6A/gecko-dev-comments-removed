const Cc = Components.classes;
const Ci = Components.interfaces;

let ioService = Cc["@mozilla.org/network/io-service;1"]
                .getService(Ci.nsIIOService);
let resProt = ioService.getProtocolHandler("resource")
              .QueryInterface(Ci.nsIResProtocolHandler);

function run_test() {
    
    let greModulesURI = ioService.newURI("resource://gre/modules/", null, null);
    resProt.setSubstitution("my-gre-modules", greModulesURI);

    
    
    let greFileSpec = ioService.newURI("modules/", null,
                                       resProt.getSubstitution("gre")).spec;
    let aliasURI = resProt.getSubstitution("my-gre-modules");
    do_check_eq(aliasURI.spec, greFileSpec);

    
    
    let greNetUtilURI = ioService.newURI("resource://gre/modules/NetUtil.jsm",
                                         null, null);
    let myNetUtilURI = ioService.newURI("resource://my-gre-modules/NetUtil.jsm",
                                        null, null);
    do_check_eq(resProt.resolveURI(greNetUtilURI),
                resProt.resolveURI(myNetUtilURI));
}
