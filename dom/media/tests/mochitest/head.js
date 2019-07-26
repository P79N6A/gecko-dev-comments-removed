



var Cc = SpecialPowers.Cc;
var Ci = SpecialPowers.Ci;
var Cr = SpecialPowers.Cr;









function runTest(aCallback, desktopSupportedOnly) {
  SimpleTest.waitForExplicitFinish();

  
  
  if(desktopSupportedOnly && (navigator.userAgent.indexOf('Android') > -1 ||
     navigator.platform === '')) {
    ok(true, navigator.userAgent + ' currently not supported');
    SimpleTest.finish();
  } else {
    SpecialPowers.pushPrefEnv({'set': [['media.peerconnection.enabled', true]]}, function () {
      try {
        aCallback();
      }
      catch (err) {
        unexpectedCallbackAndFinish(err);
      }
    });
  }
}







function unexpectedCallbackAndFinish(obj) {
  ok(false, "Unexpected error callback with " + obj);
  SimpleTest.finish();
}
