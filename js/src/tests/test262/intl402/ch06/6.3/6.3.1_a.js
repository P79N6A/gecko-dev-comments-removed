







var wellFormedCurrencyCodes = [
    "BOB",
    "EUR",
    "usd", 
    "XdR",
    "xTs"
];

wellFormedCurrencyCodes.forEach(function (code) {
    
    var format = new Intl.NumberFormat(["de-de"], {style: "currency", currency: code});
    if (format.resolvedOptions().currency !== code.toUpperCase()) {
        $ERROR("Currency " + code + " was not correctly accepted; turned into " +
            format.resolvedOptions().currency + ".");
    }
});

