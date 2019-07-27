






var dateTimeFormatCache = new Record();









function Date_toLocaleString() {
    
    var x = callFunction(std_Date_valueOf, this);
    if (Number_isNaN(x))
        return "Invalid Date";

    
    var locales = arguments.length > 0 ? arguments[0] : undefined;
    var options = arguments.length > 1 ? arguments[1] : undefined;

    
    var dateTimeFormat;
    if (locales === undefined && options === undefined) {
        
        
        if (dateTimeFormatCache.dateTimeFormat === undefined) {
            options = ToDateTimeOptions(options, "any", "all");
            dateTimeFormatCache.dateTimeFormat = intl_DateTimeFormat(locales, options);
        }
        dateTimeFormat = dateTimeFormatCache.dateTimeFormat;
    } else {
        options = ToDateTimeOptions(options, "any", "all");
        dateTimeFormat = intl_DateTimeFormat(locales, options);
    }

    
    return intl_FormatDateTime(dateTimeFormat, x);
}









function Date_toLocaleDateString() {
    
    var x = callFunction(std_Date_valueOf, this);
    if (Number_isNaN(x))
        return "Invalid Date";

    
    var locales = arguments.length > 0 ? arguments[0] : undefined;
    var options = arguments.length > 1 ? arguments[1] : undefined;

    
    var dateTimeFormat;
    if (locales === undefined && options === undefined) {
        
        
        if (dateTimeFormatCache.dateFormat === undefined) {
            options = ToDateTimeOptions(options, "date", "date");
            dateTimeFormatCache.dateFormat = intl_DateTimeFormat(locales, options);
        }
        dateTimeFormat = dateTimeFormatCache.dateFormat;
    } else {
        options = ToDateTimeOptions(options, "date", "date");
        dateTimeFormat = intl_DateTimeFormat(locales, options);
    }

    
    return intl_FormatDateTime(dateTimeFormat, x);
}









function Date_toLocaleTimeString() {
    
    var x = callFunction(std_Date_valueOf, this);
    if (Number_isNaN(x))
        return "Invalid Date";

    
    var locales = arguments.length > 0 ? arguments[0] : undefined;
    var options = arguments.length > 1 ? arguments[1] : undefined;

    
    var dateTimeFormat;
    if (locales === undefined && options === undefined) {
        
        
        if (dateTimeFormatCache.timeFormat === undefined) {
            options = ToDateTimeOptions(options, "time", "time");
            dateTimeFormatCache.timeFormat = intl_DateTimeFormat(locales, options);
        }
        dateTimeFormat = dateTimeFormatCache.timeFormat;
    } else {
        options = ToDateTimeOptions(options, "time", "time");
        dateTimeFormat = intl_DateTimeFormat(locales, options);
    }

    
    return intl_FormatDateTime(dateTimeFormat, x);
}
