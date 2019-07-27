function checkNameLookup() {
    return "global";
}

function assertWithMessage(got, expected, message) {
    assertEq(message + ": " + got, message + ": " + expected);
}



evaluate(`function testFunc() {
    assertWithMessage(checkNameLookup(), "local", "nameLookup");
    assertWithMessage(checkThisBinding(), "local", "thisBinding");
}`, { compileAndGo: false });

var obj = {
    checkNameLookup: function() {
	return "local";
    },

    checkThisBinding: function() {
	return this.checkNameLookup();
    },
};

var cloneFunc = clone(testFunc, obj);
cloneFunc();
