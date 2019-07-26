








var thisValues = [true, 42, "国際化"];

thisValues.forEach(function (value) {
    var collator = Intl.Collator.call(value);
    
    var referenceCollator = new Intl.Collator();
    if (Intl.Collator.prototype.compare.call(collator, "a", "b") !== referenceCollator.compare("a", "b")) {
        $ERROR("Collator initialized from " + value + " doesn't behave like normal collator.");
    }
    return true;
});

