







$INCLUDE("testIntl.js");

testWithIntlConstructors(function (Constructor) {
    
    
    
    var error;
    try {
        var supportedForNull = Constructor.supportedLocalesOf(null);
    } catch (e) {
        error = e;
    }
    if (error === undefined) {
        $ERROR("Null as locale list was not rejected.");
    } else if (error.name !== "TypeError") {
        $ERROR("Null as locale list was rejected with wrong error " + error.name + ".");
    }    
    
    
    var supportedForEmptyList = Constructor.supportedLocalesOf([]);
    
    if (supportedForEmptyList.length !== 0) {
        $ERROR("Internal test error: Assumption about length being 0 is invalid.");
    }

    
    var supportedForNumber = Constructor.supportedLocalesOf(5);
    if (supportedForNumber.length !== supportedForEmptyList.length) {
        $ERROR("Supported locales differ between numeric and empty list input.");
    }
    var supportedForBoolean = Constructor.supportedLocalesOf(true);
    if (supportedForBoolean.length !== supportedForEmptyList.length) {
        $ERROR("Supported locales differ between boolean and empty list input.");
    }
    
    return true;
});

