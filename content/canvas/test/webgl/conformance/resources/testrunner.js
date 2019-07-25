


































function runTestsuite(testsuite) {
	var testname;
	function testFatal(message) {
		testFailed("FATAL: " + testname + " :: " + message);
	}

	var n = 0;
	for (testname in testsuite) {
try {
		if (testname === "setup" || testname === "teardown") 
			continue;

		var test = new testsuite[testname]();

		if (n > 0 && typeof(testsuite.teardown) === "function")
			testsuite.teardown();  
		if (typeof(testsuite.setup) === "function")
			testsuite.setup(); 

		n++;

		if (typeof(test.setup) !== "function") {
			testFatal("`setup' is not a function.");
			continue;
		}
		test.setup();

		var err = gl.getError();
		switch (typeof(test.expects)) {
		case "number": 
			if (err !== test.expects) {
				testFailed(testname + " :: got GL error `" + getGLErrorAsString(gl, err) + "' instead of expected `" + getGLErrorAsString(gl, test.expects) + "'.");
				continue;
			}
			break;
		case "string": 
				testFailed(testname + " :: expected a `" + test.expects + "' exception but no exception has been caught.");
				continue;
		case "function": 
			if (err !== gl.NO_ERROR) {
				testFailed(testname + " :: got GL error `" + getGLErrorAsString(gl, err) + "' when none was expected.");
				continue;
			}
			if (test.expects() !== true) {
				testFailed(testname);
				continue;
			}
			break;
		default:
			testFatal("`expects' is neither a function or a number (GL error) but `" + typeof(test.expects) + "'.");
			continue;
		}
		testPassed(testname);
} catch (e) {
	if (test && typeof(test.expects) === "string") { 
		if (e.toString().indexOf(test.expects) === 0) 
			testPassed(testname);
		else 
			testFailed(testname + " :: caught exception `" + e + "' when a `" + test.expects + "' exception was expected.");
		continue;
	}
	testFailed(testname + " :: caught unexpected exception `" + e + "'.");
}
	}
}

