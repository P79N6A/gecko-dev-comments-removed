







$INCLUDE("testIntl.js");

var notStringOrObject = [undefined, null, true, false, 0, 5, -5, NaN];

testWithIntlConstructors(function (Constructor) {
    notStringOrObject.forEach(function (value) {
        var error;
        try {
            var supported = Constructor.supportedLocalesOf([value]);
        } catch (e) {
            error = e;
        }
        if (error === undefined) {
            $ERROR("" + value + " as locale was not rejected.");
        } else if (error.name !== "TypeError") {
            $ERROR("" + value + " as locale was rejected with wrong error " + error.name + ".");
        }
    });
    
    return true;
});

