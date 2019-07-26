








var thisValues = [true, 42, "国際化"];

thisValues.forEach(function (value) {
    var format = Intl.NumberFormat.call(value);
    
    var referenceFormat = new Intl.NumberFormat();
    if (Intl.NumberFormat.prototype.format.call(format, 12.3456) !== referenceFormat.format(12.3456)) {
        $ERROR("NumberFormat initialized from " + value + " doesn't behave like normal number format.");
    }
    return true;
});

