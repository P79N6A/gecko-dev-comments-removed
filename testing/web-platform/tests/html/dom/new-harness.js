

ReflectionHarness.catchUnexpectedExceptions = false;

ReflectionHarness.test = function(expected, actual, description) {
	test(function() {
		assert_equals(expected, actual);
	}, this.getTypeDescription() + ": " + description);
	
	
	return true;
}

ReflectionHarness.run = function(fun, description) {
	test(fun, this.getTypeDescription() + ": " + description);
}

ReflectionHarness.testException = function(exceptionName, fn, description) {
	test(function() {
		assert_throws(exceptionName, fn);
	}, this.getTypeDescription() + ": " + description);
}
