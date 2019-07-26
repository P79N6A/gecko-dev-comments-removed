







var functions = {
    toLocaleString: Date.prototype.toLocaleString,
    toLocaleDateString: Date.prototype.toLocaleDateString,
    toLocaleTimeString: Date.prototype.toLocaleTimeString
};
var invalidValues = [NaN, Infinity, -Infinity];

Object.getOwnPropertyNames(functions).forEach(function (p) {
    var f = functions[p];
    invalidValues.forEach(function (value) {
        var result = f.call(new Date(value));
        if (result !== "Invalid Date") {
            $ERROR("Date.prototype." + p + " did not return \"Invalid Date\" for " +
                value + " â€“ got " + result + " instead.");
        }
    });
});

