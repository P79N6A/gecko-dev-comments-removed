


"use strict";

Cu.import("resource://gre/modules/MobileIdentityVerificationFlow.jsm");

function verifyStrategy() {
  return Promise.resolve();
}

function cleanupStrategy() {
}

function run_test() {
  do_print("= Bug 1101444: Invalid verification code shouldn't restart " +
           "verification flow =");

  let client = new MockClient({
    
    verifyCodeError: ["INVALID", "INVALID"]
  });
  let ui = new MockUi();

  let verificationFlow = new MobileIdentityVerificationFlow({
    external: true,
    sessionToken: SESSION_TOKEN,
    msisdn: PHONE_NUMBER
  }, ui, client, verifyStrategy, cleanupStrategy);

  verificationFlow.doVerification().then(() => {
    
    
    
    client._("register").callsLength(1);
    client._("verifyCode").callsLength(3);
    
    ui._("error").callsLength(2);
  });

  do_test_finished();
};
