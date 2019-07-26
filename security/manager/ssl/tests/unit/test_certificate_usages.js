"use strict";

















do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"].getService(Ci.nsIX509CertDB);

const gNumCAs = 4;

function run_test() {
  
  for (var i = 0; i < gNumCAs; i++) {
    var ca_name = "ca-" + (i + 1);
    var ca_filename = ca_name + ".der";
    addCertFromFile(certdb, "test_certificate_usages/" + ca_filename, "CTu,CTu,CTu");
    do_print("ca_name=" + ca_name);
    var cert = certdb.findCertByNickname(null, ca_name);
  }

  run_test_in_mode(true);
  run_test_in_mode(false);
}

function run_test_in_mode(useMozillaPKIX) {
  Services.prefs.setBoolPref("security.use_mozillapkix_verification", useMozillaPKIX);
  clearOCSPCache();
  clearSessionCache();

  
  var allCAUsages = useMozillaPKIX
                  ? 'SSL CA'
                  : 'Client,Server,Sign,Encrypt,SSL CA,Status Responder';

  
  
  var ca_usages = [allCAUsages,
                   'SSL CA',
                   allCAUsages,
                   useMozillaPKIX ? ''
                                  : 'Client,Server,Sign,Encrypt,Status Responder'];

  
  var basicEndEntityUsages = useMozillaPKIX
                           ? 'Client,Server,Sign,Encrypt,Object Signer'
                           : 'Client,Server,Sign,Encrypt';
  var basicEndEntityUsagesWithObjectSigner = basicEndEntityUsages + ",Object Signer"

  
  
  var statusResponderUsages = (useMozillaPKIX ? "" : "Server,") + "Status Responder";
  var statusResponderUsagesFull
      = useMozillaPKIX ? statusResponderUsages
                       : basicEndEntityUsages + ',Object Signer,Status Responder';

  var ee_usages = [
    [ basicEndEntityUsages,
      basicEndEntityUsages,
      basicEndEntityUsages,
      '',
      statusResponderUsagesFull,
      'Client,Server',
      'Sign,Encrypt,Object Signer',
      statusResponderUsages
    ],

    [ basicEndEntityUsages,
      basicEndEntityUsages,
      basicEndEntityUsages,
      '',
      statusResponderUsagesFull,
      'Client,Server',
      'Sign,Encrypt,Object Signer',
      statusResponderUsages
    ],

    [ basicEndEntityUsages,
      basicEndEntityUsages,
      basicEndEntityUsages,
      '',
      statusResponderUsagesFull,
      'Client,Server',
      'Sign,Encrypt,Object Signer',
      statusResponderUsages
    ],

    
    
    
    
    
    
    [ useMozillaPKIX ? '' : basicEndEntityUsages,
      useMozillaPKIX ? '' : basicEndEntityUsages,
      useMozillaPKIX ? '' : basicEndEntityUsages,
      '',
      useMozillaPKIX ? '' : statusResponderUsagesFull,
      useMozillaPKIX ? '' : 'Client,Server',
      useMozillaPKIX ? '' : 'Sign,Encrypt,Object Signer',
      useMozillaPKIX ? '' : 'Server,Status Responder'
     ]
  ];

  do_check_eq(gNumCAs, ca_usages.length);

  for (var i = 0; i < gNumCAs; i++) {
    var ca_name = "ca-" + (i + 1);
    var verified = {};
    var usages = {};
    var cert = certdb.findCertByNickname(null, ca_name);
    cert.getUsagesString(true, verified, usages);
    do_print("usages.value=" + usages.value);
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
      do_print("cert usages.value=" + usages.value);
      do_check_eq(ee_usages[i][j], usages.value);
    }
  }
}
