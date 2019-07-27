


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = "head.js";

function testUnsupportedService() {
  try {
    icc.getServiceState("unsupported-service");
    ok(false, "should get exception");
  } catch (aException) {
    ok(true, "got exception: " + aException);
  }
}


startTestCommon(function() {
  let icc = getMozIcc();

  
  return icc.getServiceState("fdn")
    .then((aResult) => {
      is(aResult, true, "check fdn service state");
    })

    
    .then(() => testUnsupportedService());
});
