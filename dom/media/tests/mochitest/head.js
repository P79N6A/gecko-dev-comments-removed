



var Cc = SpecialPowers.Cc;
var Ci = SpecialPowers.Ci;
var Cr = SpecialPowers.Cr;


var FAKE_ENABLED = true;









function runTest(aCallback, desktopSupportedOnly) {
  SimpleTest.waitForExplicitFinish();

  
  
  if(desktopSupportedOnly && (navigator.userAgent.indexOf('Android') > -1 ||
     navigator.platform === '')) {
    ok(true, navigator.userAgent + ' currently not supported');
    SimpleTest.finish();
  } else {
    SpecialPowers.pushPrefEnv({'set': [
      ['media.peerconnection.enabled', true],
      ['media.navigator.permission.denied', true]]}, function () {
      try {
        aCallback();
      }
      catch (err) {
        unexpectedCallbackAndFinish(err);
      }
    });
  }
}












function getUserMedia(constraints, onSuccess, onError) {
  constraints["fake"] = FAKE_ENABLED;
  navigator.mozGetUserMedia(constraints, onSuccess, onError);
}







function unexpectedCallbackAndFinish(aObj) {
  ok(false, "Unexpected error callback with " + aObj);
  SimpleTest.finish();
}
