







var obj = new Intl.Collator();

var toStringValue = Object.prototype.toString.call(obj);
if (toStringValue !== "[object Object]") {
    $ERROR("Intl.Collator instance produces wrong [[Class]] - toString returns " + toStringValue + ".");
}

