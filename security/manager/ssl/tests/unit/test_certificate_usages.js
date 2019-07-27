"use strict";

















do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"].getService(Ci.nsIX509CertDB);

const gNumCAs = 4;

function run_test() {
  
  for (var i = 0; i < gNumCAs; i++) {
    var ca_name = "ca-" + (i + 1);
    var ca_filename = ca_name + ".der";
    addCertFromFile(certdb, "test_certificate_usages/" + ca_filename, "CTu,CTu,CTu");
    var cert = certdb.findCertByNickname(null, ca_name);
  }

  
  
  var ca_usages = ['SSL CA',
                   'SSL CA',
                   'SSL CA',
                   ''];

  
  var basicEndEntityUsages = 'Client,Server,Sign,Encrypt,Object Signer';
  var basicEndEntityUsagesWithObjectSigner = basicEndEntityUsages + ",Object Signer"

  
  
  var ee_usages = [
    [ basicEndEntityUsages,
      basicEndEntityUsages,
      basicEndEntityUsages,
      basicEndEntityUsages,
      '',
      'Status Responder',
      'Client,Server',
      'Sign,Encrypt,Object Signer',
      'Status Responder'
    ],

    [ basicEndEntityUsages,
      basicEndEntityUsages,
      basicEndEntityUsages,
      basicEndEntityUsages,
      '',
      'Status Responder',
      'Client,Server',
      'Sign,Encrypt,Object Signer',
      'Status Responder'
    ],

    [ basicEndEntityUsages,
      basicEndEntityUsages,
      basicEndEntityUsages,
      basicEndEntityUsages,
      '',
      'Status Responder',
      'Client,Server',
      'Sign,Encrypt,Object Signer',
      'Status Responder'
    ],

    
    
    
    
    
    
    [ '',
      '',
      '',
      '',
      '',
      '',
      '',
      '',
      ''
     ]
  ];

  do_check_eq(gNumCAs, ca_usages.length);

  for (var i = 0; i < gNumCAs; i++) {
    var ca_name = "ca-" + (i + 1);
    var verified = {};
    var usages = {};
    var cert = certdb.findCertByNickname(null, ca_name);
    cert.getUsagesString(true, verified, usages);
    do_check_eq(ca_usages[i], usages.value);
    if (ca_usages[i].indexOf('SSL CA') != -1) {
      checkCertErrorGeneric(certdb, cert, 0, certificateUsageVerifyCA);
    }
    
    for (var j = 0; j < ee_usages[i].length; j++) {
      var ee_name = "ee-" + (j + 1) + "-" + ca_name;
      var ee_filename = ee_name + ".der";
      addCertFromFile(certdb, "test_certificate_usages/" + ee_filename, ",,");
      var ee_cert;
      ee_cert = certdb.findCertByNickname(null, ee_name);
      var verified = {};
      var usages = {};
      ee_cert.getUsagesString(true, verified, usages);
      do_check_eq(ee_usages[i][j], usages.value);
    }
  }
}
