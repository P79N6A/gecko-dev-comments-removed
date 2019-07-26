







var dates = [
    0, 
    -62151602400000, 
    -8640000000000000 
];

var format = new Intl.DateTimeFormat(["en-US"], {year: "numeric", month: "long", timeZone: "UTC"});


if (format.resolvedOptions().calendar !== "gregory") {
    $ERROR("Internal error: Didn't find Gregorian calendar");
}

dates.forEach(function (date) {
    var year = new Date(date).getUTCFullYear();
    var expectedYear = year <= 0 ? 1 - year : year;
    var expectedYearString = expectedYear.toLocaleString(["en-US"], {useGrouping: false});
    var dateString = format.format(date);
    if (dateString.indexOf(expectedYearString) === -1) {
        $ERROR("Formatted year doesn't contain expected year â€“ expected " +
            expectedYearString + ", got " + dateString + ".");
    }
});

