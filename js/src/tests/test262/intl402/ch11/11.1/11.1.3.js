







var obj = new Intl.NumberFormat();

var actualPrototype = Object.getPrototypeOf(obj);
if (actualPrototype !== Intl.NumberFormat.prototype) {
    $ERROR("Prototype of object constructed by Intl.NumberFormat isn't Intl.NumberFormat.prototype; got " + actualPrototype);
}

if (!Object.isExtensible(obj)) {
    $ERROR("Object constructed by Intl.NumberFormat must be extensible.");
}

