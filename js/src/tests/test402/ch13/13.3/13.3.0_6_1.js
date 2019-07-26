







var functions = {
    toLocaleString: Date.prototype.toLocaleString,
    toLocaleDateString: Date.prototype.toLocaleDateString,
    toLocaleTimeString: Date.prototype.toLocaleTimeString
};
var locales = [null, [NaN], ["i"], ["de_DE"]];
var options = [
    {localeMatcher: null},
    {timeZone: "invalid"},
    {hour: "long"},
    {formatMatcher: "invalid"}
];

Object.getOwnPropertyNames(functions).forEach(function (p) {
    var f = functions[p];
    locales.forEach(function (locales) {
        var referenceError, error;
        try {
            var format = new Intl.DateTimeFormat(locales);
        } catch (e) {
            referenceError = e;
        }
        if (referenceError === undefined) {
            $ERROR("Internal error: Expected exception was not thrown by Intl.DateTimeFormat for locales " + locales + ".");
        }
        
        try {
            var result = f.call(new Date(), locales);
        } catch (e) {
            error = e;
        }
        if (error === undefined) {
            $ERROR("Date.prototype." + p + " didn't throw exception for locales " + locales + ".");
        } else if (error.name !== referenceError.name) {
            $ERROR("Date.prototype." + p + " threw exception " + error.name +
                " for locales " + locales + "; expected " + referenceError.name + ".");
        }
    });
    
    options.forEach(function (options) {
        var referenceError, error;
        try {
            var format = new Intl.DateTimeFormat([], options);
        } catch (e) {
            referenceError = e;
        }
        if (referenceError === undefined) {
            $ERROR("Internal error: Expected exception was not thrown by Intl.DateTimeFormat for options " +
                JSON.stringify(options) + ".");
        }
        
        try {
            var result = f.call(new Date(), [], options);
        } catch (e) {
            error = e;
        }
        if (error === undefined) {
            $ERROR("Date.prototype." + p + " didn't throw exception for options " +
                JSON.stringify(options) + ".");
        } else if (error.name !== referenceError.name) {
            $ERROR("Date.prototype." + p + " threw exception " + error.name +
                " for options " + JSON.stringify(options) + "; expected " + referenceError.name + ".");
        }
    });
});

