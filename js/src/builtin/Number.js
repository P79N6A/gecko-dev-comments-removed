






var numberFormatCache = new Record();









function Number_toLocaleString() {
    
    var x = callFunction(std_Number_valueOf, this);

    
    var locales = arguments.length > 0 ? arguments[0] : undefined;
    var options = arguments.length > 1 ? arguments[1] : undefined;

    
    var numberFormat;
    if (locales === undefined && options === undefined) {
        
        
        if (numberFormatCache.numberFormat === undefined)
            numberFormatCache.numberFormat = intl_NumberFormat(locales, options);
        numberFormat = numberFormatCache.numberFormat;
    } else {
        numberFormat = intl_NumberFormat(locales, options);
    }

    
    return intl_FormatNumber(numberFormat, x);
}
