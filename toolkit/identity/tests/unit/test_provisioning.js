


"use strict";

Cu.import("resource://gre/modules/identity/IdentityProvider.jsm");

function check_provision_flow_done(provId) {
  do_check_null(IdentityProvider._provisionFlows[provId]);
}

function test_begin_provisioning() {
  do_test_pending();

  setup_provisioning(
    TEST_USER,
    function(caller) {
      
      IdentityProvider.beginProvisioning(caller);
    }, function() {},
    {
      beginProvisioningCallback: function(email, duration_s) {
        do_check_eq(email, TEST_USER);
        do_check_true(duration_s > 0);
        do_check_true(duration_s <= (24 * 3600));

        do_test_finished();
        run_next_test();
      }
    });
}

function test_raise_provisioning_failure() {
  do_test_pending();
  let _callerId = null;

  setup_provisioning(
    TEST_USER,
    function(caller) {
      
      _callerId = caller.id;
      IdentityProvider.beginProvisioning(caller);
    }, function(err) {
      
      do_check_neq(err, null);
      do_check_true(err.indexOf("can't authenticate this email") > -1);

      do_test_finished();
      run_next_test();
    },
    {
      beginProvisioningCallback: function(email, duration_s) {
        
        IdentityProvider.raiseProvisioningFailure(_callerId, "can't authenticate this email");
      }
    });
}

function test_genkeypair_before_begin_provisioning() {
  do_test_pending();

  setup_provisioning(
    TEST_USER,
    function(caller) {
      
      IdentityProvider.genKeyPair(caller.id);
    },
    
    function(err) {
      do_check_neq(err, null);

      do_test_finished();
      run_next_test();
    },
    {
      
      genKeyPairCallback: function(pk) {
        
        do_check_true(false);

        do_test_finished();
        run_next_test();
      }
    }
  );
}

function test_genkeypair() {
  do_test_pending();
  let _callerId = null;

  setup_provisioning(
    TEST_USER,
    function(caller) {
      _callerId = caller.id;
      IdentityProvider.beginProvisioning(caller);
    },
    function(err) {
      
      do_check_true(false);

      do_test_finished();
      run_next_test();
    },
    {
      beginProvisioningCallback: function(email, time_s) {
        IdentityProvider.genKeyPair(_callerId);
      },
      genKeyPairCallback: function(kp) {
        do_check_neq(kp, null);

        
        do_test_finished();
        run_next_test();
      }
    }
  );
}




function test_register_certificate_before_genkeypair() {
  do_test_pending();
  let _callerID = null;

  setup_provisioning(
    TEST_USER,
    function(caller) {
      
      _callerID = caller.id;
      IdentityProvider.beginProvisioning(caller);
    },
    
    function(err) {
      do_check_neq(err, null);

      do_test_finished();
      run_next_test();
    },
    {
      beginProvisioningCallback: function(email, duration_s) {
        
        IdentityProvider.registerCertificate(_callerID, "fake-cert");
      }
    }
  );
}

function test_register_certificate() {
  do_test_pending();
  let _callerId = null;

  setup_provisioning(
    TEST_USER,
    function(caller) {
      _callerId = caller.id;
      IdentityProvider.beginProvisioning(caller);
    },
    function(err) {
      
      do_check_null(err);

      
      let identity = get_idstore().fetchIdentity(TEST_USER);
      do_check_neq(identity,null);
      do_check_eq(identity.cert, "fake-cert-42");

      do_execute_soon(function check_done() {
        
        check_provision_flow_done(_callerId);

        do_test_finished();
        run_next_test();
      });
    },
    {
      beginProvisioningCallback: function(email, duration_s) {
        IdentityProvider.genKeyPair(_callerId);
      },
      genKeyPairCallback: function(pk) {
        IdentityProvider.registerCertificate(_callerId, "fake-cert-42");
      }
    }
  );
}


function test_get_assertion_after_provision() {
  do_test_pending();
  let _callerId = null;

  setup_provisioning(
    TEST_USER,
    function(caller) {
      _callerId = caller.id;
      IdentityProvider.beginProvisioning(caller);
    },
    function(err) {
      
      do_check_null(err);

      
      let identity = get_idstore().fetchIdentity(TEST_USER);
      do_check_neq(identity,null);
      do_check_eq(identity.cert, "fake-cert-42");

      do_execute_soon(function check_done() {
        
        check_provision_flow_done(_callerId);

        do_test_finished();
        run_next_test();
      });
    },
    {
      beginProvisioningCallback: function(email, duration_s) {
        IdentityProvider.genKeyPair(_callerId);
      },
      genKeyPairCallback: function(pk) {
        IdentityProvider.registerCertificate(_callerId, "fake-cert-42");
      }
    }
  );

}

let TESTS = [];

TESTS.push(test_begin_provisioning);
TESTS.push(test_raise_provisioning_failure);
TESTS.push(test_genkeypair_before_begin_provisioning);
TESTS.push(test_genkeypair);
TESTS.push(test_register_certificate_before_genkeypair);
TESTS.push(test_register_certificate);
TESTS.push(test_get_assertion_after_provision);

TESTS.forEach(add_test);

function run_test() {
  run_next_test();
}
