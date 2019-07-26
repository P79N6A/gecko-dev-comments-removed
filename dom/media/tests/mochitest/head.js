



var Cc = SpecialPowers.Cc;
var Ci = SpecialPowers.Ci;
var Cr = SpecialPowers.Cr;









function runTest(aCallback, desktopSupportedOnly) {
  SimpleTest.waitForExplicitFinish();

  
  
  if(desktopSupportedOnly && (navigator.platform === 'Android' ||
     navigator.platform === '')) {
    ok(true, navigator.platform + ' currently not supported');
    SimpleTest.finish();
  } else {
    SpecialPowers.pushPrefEnv({'set': [['media.peerconnection.enabled', true]]},
      aCallback);
  }
}







function unexpectedCallbackAndFinish(obj) {
  ok(false, "Unexpected error callback with " + obj);
  SimpleTest.finish();
}
