







var defaultLocale = new Intl.NumberFormat().resolvedOptions().locale;

function expectError(f) {
    var error;
    try {
        f();
    } catch (e) {
        error = e;
    }
    if (error === undefined) {
        $ERROR("Invalid currency value " + value + " was not rejected.");
    } else if (error.name !== "TypeError") {
        $ERROR("Invalid currency value " + value + " was rejected with wrong error " + error.name + ".");
    }
}

expectError(function () {
        return new Intl.NumberFormat([defaultLocale], {style: "currency"});
});
expectError(function () {
        return new Intl.NumberFormat([defaultLocale + "-u-cu-krw"], {style: "currency"});
});

