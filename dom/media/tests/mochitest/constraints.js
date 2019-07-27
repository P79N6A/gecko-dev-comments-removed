










var common_tests = [
  
  { message: "unknown required constraint on video fails",
    constraints: { video: { somethingUnknown:0, require:["somethingUnknown"] },
                   fake: true },
    error: "NotFoundError" },
  { message: "unknown required constraint on audio fails",
    constraints: { audio: { somethingUnknown:0, require:["somethingUnknown"] },
                   fake: true },
    error: "NotFoundError" },
  { message: "video overconstrained by facingMode fails",
    constraints: { video: { facingMode:'left', require:["facingMode"] },
                   fake: true },
    error: "NotFoundError" },
  { message: "audio overconstrained by facingMode fails",
    constraints: { audio: { facingMode:'left', require:["facingMode"] },
                   fake: true },
    error: "NotFoundError" },
  { message: "Success-path: optional video facingMode + audio ignoring facingMode",
    constraints: { fake: true,
                   audio: { facingMode:'left',
                            foo:0,
                            advanced: [{ facingMode:'environment' },
                                       { facingMode:'user' },
                                       { bar:0 }] },
                   video: { 
                            
                            
                            facingMode:'left',
                            foo:0,
                            advanced: [{ facingMode:'environment' },
                                       { facingMode:'user' },
                                       { bar:0 }] } },
    error: null }
];







function testConstraints(tests) {
  function testgum(p, test) {
    return p.then(function() {
      return navigator.mediaDevices.getUserMedia(test.constraints);
    })
    .then(function() {
      is(null, test.error, test.message);
    }, function(reason) {
      is(reason.name, test.error, test.message + ": " + reason.message);
    });
  }

  var p = new Promise(function(resolve) { resolve(); });
  tests.forEach(function(test) {
    p = testgum(p, test);
  });
  p.catch(function(reason) {
    ok(false, "Unexpected failure: " + reason.message);
  })
  .then(SimpleTest.finish);
}
