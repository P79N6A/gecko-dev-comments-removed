







var validTimeZoneNames = [
    "UTC",
    "utc" 
];

validTimeZoneNames.forEach(function (name) {
    
    var format = new Intl.DateTimeFormat(["de-de"], {timeZone: name});
    if (format.resolvedOptions().timeZone !== name.toUpperCase()) {
        $ERROR("Time zone name " + name + " was not correctly accepted; turned into " +
            format.resolvedOptions().timeZone + ".");
    }
});

