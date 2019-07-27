



"use strict";



do_get_profile(); 
const certDB = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);

function certFromFile(filename) {
  return constructCertFromFile(`test_certviewer_invalid_oids/${filename}.pem`);
}

function test(certFilename, expectedOIDText) {
  let cert = certFromFile(certFilename);
  let certDumpTree = Cc["@mozilla.org/security/nsASN1Tree;1"]
                       .createInstance(Ci.nsIASN1Tree);
  certDumpTree.loadASN1Structure(cert.ASN1Structure);
  let actualOIDText = certDumpTree.getDisplayData(9);

  equal(actualOIDText, expectedOIDText,
        "Actual and expected OID text should match");
}

function run_test() {
  test("bug483440-attack2b",
       "Object Identifier (2 5 4 Unknown) = www.bank.com\n" +
       "OU = Hacking Division\n" +
       "CN = www.badguy.com\nO = Badguy Inc\n");

  test("bug483440-pk10oflo",
       "Object Identifier (2 5 4 Unknown) = www.bank.com\n" +
       "OU = Hacking Division\n" +
       "CN = www.badguy.com\nO = Badguy Inc\n");

  test("bug483440-attack7",

       
       "Object Identifier (2 5 4 2147483649) = attack1\n" +

       
       "Object Identifier (2 5 4 Unknown) = attack2\n" +

       
       "Object Identifier (2 5 4 Unknown) = attack3\n" +

       
       "Object Identifier (2 5 4 3 Unknown) = attack4\n" +

       
       "Object Identifier (2 5 4 268435455) = attack5\n" +

       
       "Object Identifier (Unknown 3) = attack6\n" +

       
       
       "Object Identifier (2 14677 4 3) = attack7\n");
}
