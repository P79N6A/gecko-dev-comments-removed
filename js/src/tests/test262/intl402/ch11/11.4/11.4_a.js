







var obj = new Intl.NumberFormat();

var toStringValue = Object.prototype.toString.call(obj);
if (toStringValue !== "[object Object]") {
    $ERROR("Intl.NumberFormat instance produces wrong [[Class]] - toString returns " + toStringValue + ".");
}

