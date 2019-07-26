








var numberFormatter = new Intl.NumberFormat();
var percentFormatter = new Intl.NumberFormat(undefined, {style: 'percent'});

var formattedTwenty = numberFormatter.format(20);
var formattedTwentyPercent = percentFormatter.format(0.20);



if (formattedTwentyPercent.indexOf(formattedTwenty) === -1) {
    $ERROR("Intl.NumberFormat's formatting of 20% does not include a " +
        "formatting of 20 as a substring.");
}


if (percentFormatter.format(0.011) === percentFormatter.format(0.02)) {
    $ERROR('Intl.NumberFormat is formatting 1.1% and 2% the same way.');
}

