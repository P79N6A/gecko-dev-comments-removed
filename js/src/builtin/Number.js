






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


function Number_isFinite(num) {
    if (typeof num !== "number")
        return false;
    return num - num === 0;
}


function Number_isNaN(num) {
    if (typeof num !== "number")
        return false;
    return num !== num;
}


function Number_isSafeInteger(number) {
    
    if (typeof number !== 'number')
        return false;

    
    if (!Number_isFinite(number))
        return false;

    
    var integer = ToInteger(number);

    
    if (integer !== number)
        return false;

    
    if (std_Math_abs(integer) <= 9007199254740991)
        return true;

    
    return false;
}

function Global_isNaN(number) {
    return Number_isNaN(ToNumber(number));
}

function Global_isFinite(number){
    return Number_isFinite(ToNumber(number));
}
