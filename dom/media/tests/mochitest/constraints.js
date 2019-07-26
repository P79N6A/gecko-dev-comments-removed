










var common_tests = [
  
  { message: "unknown required constraint on video fails",
    constraints: { video: { somethingUnknown:0, require:["somethingUnknown"] } },
    error: "NO_DEVICES_FOUND" },
  { message: "unknown required constraint on audio fails",
    constraints: { audio: { somethingUnknown:0, require:["somethingUnknown"] } },
    error: "NO_DEVICES_FOUND" },
  { message: "missing required constraint on video fails",
    constraints: { video: { require:["facingMode"] } },
    error: "NO_DEVICES_FOUND" },
  { message: "missing required constraint on audio fails",
    constraints: { audio: { require:["facingMode"] } },
    error: "NO_DEVICES_FOUND" },
  { message: "video overconstrained by facingMode fails",
    constraints: { video: { facingMode:'left', require:["facingMode"] } },
    error: "NO_DEVICES_FOUND" },
  { message: "audio overconstrained by facingMode fails",
    constraints: { audio: { facingMode:'left', require:["facingMode"] } },
    error: "NO_DEVICES_FOUND" },
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
  var i = 0;
  next();

  function Success() {
    ok(!tests[i].error, tests[i].message);
    i++;
    next();
  }
  function Failure(err) {
    ok(tests[i].error? (err === tests[i].error) : false,
       tests[i].message + " (err=" + err + ")");
    i++;
    next();
  }
  function next() {
    if (i < tests.length) {
      navigator.mozGetUserMedia(tests[i].constraints, Success, Failure);
    } else {
      SimpleTest.finish();
    }
  }
};
