



const traceback = require("sdk/console/traceback");

exports.test_no_args = function(test) {
  var passed = false;
  try {
    var oops = require(); 
  } catch(e) {
    let msg = e.toString();
    test.assertEqual(msg.indexOf("Error: you must provide a module name when calling require() from "), 0);
    test.assertNotEqual(msg.indexOf("test-require"), -1, msg);
    
    
    
    if (0) {
      let tb = traceback.fromException(e);
      let lastFrame = tb[tb.length-1];
      test.assertNotEqual(lastFrame.filename.indexOf("test-require.js"), -1,
                          lastFrame.filename);
      test.assertEqual(lastFrame.lineNo, 6);
      test.assertEqual(lastFrame.funcName, "??");
    }
    passed = true;
  }
  test.assert(passed, 'require() with no args should raise helpful error');
};
