







$INCLUDE("testIntl.js");

testWithIntlConstructors(function (Constructor) {
    var obj, error;
    
    
    obj = new Constructor();
    try {
        Intl.NumberFormat.call(obj);
    } catch (e) {
        error = e;
    }
    if (error === undefined) {
        $ERROR("Re-initializing object created with \"new\" as NumberFormat was not rejected.");
    } else if (error.name !== "TypeError") {
        $ERROR("Re-initializing object created with \"new\" as NumberFormat was rejected with wrong error " + error.name + ".");
    }
    
    
    obj = Constructor.call({});
    error = undefined;
    try {
        Intl.NumberFormat.call(obj);
    } catch (e) {
        error = e;
    }
    if (error === undefined) {
        $ERROR("Re-initializing object created with constructor as function as NumberFormat was not rejected.");
    } else if (error.name !== "TypeError") {
        $ERROR("Re-initializing object created with constructor as function as NumberFormat was rejected with wrong error " + error.name + ".");
    }
    
    return true;
});

