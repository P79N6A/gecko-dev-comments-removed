








var thisValues = ["a", "t", "u", "undefined", "UNDEFINED", "nicht definiert", "xyz", "未定义"];

var i;
for (i = 0; i < thisValues.length; i++) {
    var thisValue = thisValues[i];
    if (thisValue.localeCompare() !== thisValue.localeCompare(undefined)) {
        $ERROR("String.prototype.localeCompare does not treat missing 'that' argument as undefined.");
    }
    if (thisValue.localeCompare(undefined) !== thisValue.localeCompare("undefined")) {
        $ERROR("String.prototype.localeCompare does not treat undefined 'that' argument as \"undefined\".");
    }
}

