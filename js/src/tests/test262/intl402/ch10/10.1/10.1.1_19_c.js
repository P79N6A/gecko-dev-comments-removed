








$INCLUDE("testIntl.js");

var options = [
    {key: "kn", property: "numeric", type: "boolean", values: [true, false]},
    {key: "kf", property: "caseFirst", type: "string", values: ["upper", "lower", "false"]}
];

options.forEach(function (option) {
    var defaultLocale = new Intl.Collator().resolvedOptions().locale;
    var collator, opt, result;
    
    
    var supportedValues = [];
    option.values.forEach(function (value) {
        opt = {};
        opt[option.property] = value;
        collator = new Intl.Collator([defaultLocale], opt);
        result = collator.resolvedOptions()[option.property];
        if (result !== undefined && supportedValues.indexOf(result) === -1) {
            supportedValues.push(result);
        }
    });
    
    
    supportedValues.forEach(function (value) {
        collator = new Intl.Collator([defaultLocale + "-u-" + option.key + "-" + value]);
        result = collator.resolvedOptions()[option.property];
        if (result !== value) {
            $ERROR("Property " + option.property + " couldn't be set through locale extension key " +
                option.key + "; requested value: " + value + "; actual value: " + result + ".");
        }
    });
    
    
    supportedValues.forEach(function (value) {
        var otherValue;
        option.values.forEach(function (possibleValue) {
            if (possibleValue !== value) {
                otherValue = possibleValue;
            }
        });
        if (otherValue !== undefined) {
            opt = {};
            opt[option.property] = value;
            collator = new Intl.Collator([defaultLocale + "-u-" + option.key + "-" + otherValue], opt);
            result = collator.resolvedOptions()[option.property];
            if (result !== value) {
                $ERROR("Options value for property " + option.property + " doesn't override locale extension key " +
                    option.key + "; requested value: " + value + "; actual value: " + result + ".");
            }
        }
    });
});

