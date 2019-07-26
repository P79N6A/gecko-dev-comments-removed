








var thisValues = [true, 42, "国際化"];

thisValues.forEach(function (value) {
    var format = Intl.DateTimeFormat.call(value);
    
    var referenceFormat = new Intl.DateTimeFormat();
    if (Intl.DateTimeFormat.prototype.format.call(format, new Date(111111111)) !== referenceFormat.format(new Date(111111111))) {
        $ERROR("DateTimeFormat initialized from " + value + " doesn't behave like normal date-time format.");
    }
    return true;
});

