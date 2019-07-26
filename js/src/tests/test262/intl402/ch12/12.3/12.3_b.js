









var functions = {
    "format getter": Object.getOwnPropertyDescriptor(Intl.DateTimeFormat.prototype, "format").get,
    resolvedOptions: Intl.DateTimeFormat.prototype.resolvedOptions
};
var invalidTargets = [undefined, null, true, 0, "DateTimeFormat", [], {}];

Object.getOwnPropertyNames(functions).forEach(function (functionName) {
    var f = functions[functionName];
    invalidTargets.forEach(function (target) {
        var error;
        try {
            f.call(target);
        } catch (e) {
            error = e;
        }
        if (error === undefined) {
            $ERROR("Calling " + functionName + " on " + target + " was not rejected.");
        } else if (error.name !== "TypeError") {
            $ERROR("Calling " + functionName + " on " + target + " was rejected with wrong error " + error.name + ".");
        }
    });
});

