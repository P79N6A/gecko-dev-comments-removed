







var obj = new Intl.DateTimeFormat();

var toStringValue = Object.prototype.toString.call(obj);
if (toStringValue !== "[object Object]") {
    $ERROR("Intl.DateTimeFormat instance produces wrong [[Class]] - toString returns " + toStringValue + ".");
}

