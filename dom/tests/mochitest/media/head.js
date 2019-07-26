



var Cc = SpecialPowers.Cc;
var Ci = SpecialPowers.Ci;
var Cr = SpecialPowers.Cr;








function runTest(aCallback) {
  SimpleTest.waitForExplicitFinish();

  SpecialPowers.pushPrefEnv({'set': [['media.peerconnection.enabled', true],
                            ['media.navigator.permission.disabled', true]]},
                            aCallback);
}
