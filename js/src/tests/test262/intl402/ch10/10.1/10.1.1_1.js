







$INCLUDE("testIntl.js");

testWithIntlConstructors(function (Constructor) {
    var obj, error;
    
    
    obj = new Constructor();
    try {
        Intl.Collator.call(obj);
    } catch (e) {
        error = e;
    }
    if (error === undefined) {
        $ERROR("Re-initializing object created with \"new\" as Collator was not rejected.");
    } else if (error.name !== "TypeError") {
        $ERROR("Re-initializing object created with \"new\" as Collator was rejected with wrong error " + error.name + ".");
    }
    
    
    obj = Constructor.call({});
    error = undefined;
    try {
        Intl.Collator.call(obj);
    } catch (e) {
        error = e;
    }
    if (error === undefined) {
        $ERROR("Re-initializing object created with constructor as function as Collator was not rejected.");
    } else if (error.name !== "TypeError") {
        $ERROR("Re-initializing object created with constructor as function as Collator was rejected with wrong error " + error.name + ".");
    }
    
    return true;
});

