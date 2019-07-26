"use strict";

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;
let Cr = Components.results;
















let tempScope = {};
Cu.import("resource://gre/modules/NetUtil.jsm", tempScope);
let NetUtil = tempScope.NetUtil;


Cu.import("resource://gre/modules/FileUtils.jsm"); 
Cu.import("resource://gre/modules/Services.jsm");  

do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"].getService(Ci.nsIX509CertDB);

function readFile(file) {
  let fstream = Cc["@mozilla.org/network/file-input-stream;1"]
                  .createInstance(Ci.nsIFileInputStream);
  fstream.init(file, -1, 0, 0);
  let data = NetUtil.readInputStreamToString(fstream, fstream.available());
  fstream.close();
  return data;
}

var ca_usages = ['Client,Server,Sign,Encrypt,SSL CA,Status Responder',
                 'SSL CA',
                 'Client,Server,Sign,Encrypt,SSL CA,Status Responder',





















                 'Client,Server,Sign,Encrypt,Status Responder'];

var ee_usages = [
                  ['Client,Server,Sign,Encrypt',
                   'Client,Server,Sign,Encrypt',
                   'Client,Server,Sign,Encrypt',
                   '',
                   'Client,Server,Sign,Encrypt,Object Signer,Status Responder',
                   'Client,Server',
                   'Sign,Encrypt,Object Signer',
                   'Server,Status Responder'
                   ],
                  ['Client,Server,Sign,Encrypt',
                   'Client,Server,Sign,Encrypt',
                   'Client,Server,Sign,Encrypt',
                   '',
                   'Client,Server,Sign,Encrypt,Object Signer,Status Responder',
                   'Client,Server',
                   'Sign,Encrypt,Object Signer',
                   'Server,Status Responder'
                   ],
                  ['Client,Server,Sign,Encrypt',
                   'Client,Server,Sign,Encrypt',
                   'Client,Server,Sign,Encrypt',
                   '',
                   'Client,Server,Sign,Encrypt,Object Signer,Status Responder',
                   'Client,Server',
                   'Sign,Encrypt,Object Signer',
                   'Server,Status Responder'
                  ],





                  ['Client,Server,Sign,Encrypt',
                   'Client,Server,Sign,Encrypt',
                   'Client,Server,Sign,Encrypt',
                   '',
                   'Client,Server,Sign,Encrypt,Object Signer,Status Responder',
                   'Client,Server',
                   'Sign,Encrypt,Object Signer',
                   'Server,Status Responder'
                  ]
                ];


function run_test() {
  
  for (var i = 0; i < ca_usages.length; i++) {
    var ca_name = "ca-" + (i + 1);
    var ca_filename = ca_name + ".der";
    var root_cert_der =
      do_get_file("test_certificate_usages/" + ca_filename, false);
    var der = readFile(root_cert_der);
    certdb.addCert(der, "CTu,CTu,CTu", ca_name);

    do_print("ca_name=" + ca_name);
    var cert;
    cert = certdb.findCertByNickname(null, ca_name);

    var verified = {};
    var usages = {};
    cert.getUsagesString(true, verified, usages);
    do_print("usages.value=" + usages.value);
    do_check_eq(ca_usages[i], usages.value);

    
    for (var j = 0; j < ee_usages[i].length; j++) {
      var ee_name = "ee-" + (j + 1) + "-" + ca_name;
      var ee_filename = ee_name + ".der";
      
      var ee_cert_der =
        do_get_file("test_certificate_usages/" + ee_filename, false);
      var der = readFile(ee_cert_der);
      certdb.addCert(der, ",,", ee_name);
      var ee_cert;
      ee_cert = certdb.findCertByNickname(null, ee_name);
      var verified = {};
      var usages = {};
      ee_cert.getUsagesString(true, verified, usages);
      do_print("cert usages.value=" + usages.value);
      do_check_eq(ee_usages[i][j], usages.value);
    }

  }

}
