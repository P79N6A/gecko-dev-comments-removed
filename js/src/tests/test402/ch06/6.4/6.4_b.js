







var invalidTimeZoneNames = [
    "",
    "MEZ", 
    "Pacific Time", 
    "cnsha", 
    "invalid", 
    "Europe/İstanbul", 
    "asıa/baku", 
    "europe/brußels"  
];

invalidTimeZoneNames.forEach(function (name) {
    var error;
    try {
        
        var format = new Intl.DateTimeFormat(["de-de"], {timeZone: name});
    } catch (e) {
        error = e;
    }
    if (error === undefined) {
        $ERROR("Invalid time zone name " + name + " was not rejected.");
    } else if (error.name !== "RangeError") {
        $ERROR("Invalid time zone name " + name + " was rejected with wrong error " + error.name + ".");
    }
});

