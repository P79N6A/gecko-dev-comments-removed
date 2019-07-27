"use strict";












do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);

function load_cert(name, trust) {
  let filename = "test_intermediate_basic_usage_constraints/" + name + ".der";
  addCertFromFile(certdb, filename, trust);
}

function test_cert_for_usages(certChainNicks, expected_usages_string) {
  let certs = [];
  for (let i in certChainNicks) {
    let certNick = certChainNicks[i];
    let certDER = readFile(do_get_file(
                             "test_intermediate_basic_usage_constraints/"
                                + certNick + ".der"), false);
    certs.push(certdb.constructX509(certDER, certDER.length));
  }

  let cert = certs[0];
  let verified = {};
  let usages = {};
  cert.getUsagesString(true, verified, usages);
  do_print("usages.value = " + usages.value);
  do_check_eq(expected_usages_string, usages.value);
}

function run_test() {
  
  
  let ee_usage1 = 'Client,Server,Sign,Encrypt,Object Signer';

  
  
  let ca_usage1 = "SSL CA";

  
  let ca_name = "ca";
  load_cert(ca_name, "CTu,CTu,CTu");
  do_print("ca_name = " + ca_name);
  test_cert_for_usages([ca_name], ca_usage1);

  
  test_cert_for_usages(["int-no-extensions"], ee_usage1);

  
  
  test_cert_for_usages(["ee-int-no-extensions", "int-no-extensions"], "");

  
  test_cert_for_usages(["int-not-a-ca"], ee_usage1);

  
  test_cert_for_usages(["ee-int-not-a-ca", "int-not-a-ca"], "");

  
  
  test_cert_for_usages(["int-cA-FALSE-asserts-keyCertSign"], ee_usage1);
  test_cert_for_usages(["ee-int-cA-FALSE-asserts-keyCertSign",
                        "int-cA-FALSE-asserts-keyCertSign"], "");


  
  test_cert_for_usages(["int-limited-depth"], ca_usage1);

  
  
  test_cert_for_usages(["ee-int-limited-depth", "int-limited-depth"],
                       ee_usage1);

  
  
  
  
  
  
  
  test_cert_for_usages(["int-limited-depth-invalid", "int-limited-depth"], "");
  test_cert_for_usages(["ee-int-limited-depth-invalid",
                        "int-limited-depth-invalid",
                        "int-limited-depth"],
                       "");

  
  test_cert_for_usages(["int-valid-ku-no-eku"], "SSL CA");
  test_cert_for_usages(["ee-int-valid-ku-no-eku", "int-valid-ku-no-eku"],
                       ee_usage1);

  
  
  
  
  test_cert_for_usages(["int-bad-ku-no-eku"], "");
  test_cert_for_usages(["ee-int-bad-ku-no-eku", "int-bad-ku-no-eku"], "");

  
  
  test_cert_for_usages(["int-no-ku-no-eku"], ca_usage1);
  test_cert_for_usages(["ee-int-no-ku-no-eku", "int-no-ku-no-eku"], ee_usage1);

  
  
  test_cert_for_usages(["int-valid-ku-server-eku"], "SSL CA");
  test_cert_for_usages(["ee-int-valid-ku-server-eku",
                        "int-valid-ku-server-eku"], "Client,Server");

  
  
  test_cert_for_usages(["int-bad-ku-server-eku"], "");
  test_cert_for_usages(["ee-int-bad-ku-server-eku", "int-bad-ku-server-eku"],
                       "");

  
  
  test_cert_for_usages(["int-no-ku-server-eku"], "SSL CA");
  test_cert_for_usages(["ee-int-no-ku-server-eku", "int-no-ku-server-eku"],
                       "Client,Server");
}
