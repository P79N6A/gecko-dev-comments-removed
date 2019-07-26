







var invalidValues = [NaN, Infinity, -Infinity];

var format = new Intl.DateTimeFormat();

invalidValues.forEach(function (value) {
    var error;
    try {
        var result = format.format(value);
    } catch (e) {
        error = e;
    }
    if (error === undefined) {
        $ERROR("Invalid value " + value + " was not rejected.");
    } else if (error.name !== "RangeError") {
        $ERROR("Invalid value " + value + " was rejected with wrong error " + error.name + ".");
    }
});

