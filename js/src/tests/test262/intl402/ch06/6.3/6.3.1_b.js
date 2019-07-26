







var invalidCurrencyCodes = [
    "",
    "€",
    "$",
    "SFr.",
    "DM",
    "KR₩",
    "702",
    "ßP",
    "ınr"
];

invalidCurrencyCodes.forEach(function (code) {
    var error;
    try {
        
        var format = new Intl.NumberFormat(["de-de"], {style: "currency", currency: code});
    } catch (e) {
        error = e;
    }
    if (error === undefined) {
        $ERROR("Invalid currency code '" + code + "' was not rejected.");
    } else if (error.name !== "RangeError") {
        $ERROR("Invalid currency code '" + code + "' was rejected with wrong error " + error.name + ".");
    }
});

