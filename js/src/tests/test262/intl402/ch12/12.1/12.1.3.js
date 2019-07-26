







var obj = new Intl.DateTimeFormat();

var actualPrototype = Object.getPrototypeOf(obj);
if (actualPrototype !== Intl.DateTimeFormat.prototype) {
    $ERROR("Prototype of object constructed by Intl.DateTimeFormat isn't Intl.DateTimeFormat.prototype; got " + actualPrototype);
}

if (!Object.isExtensible(obj)) {
    $ERROR("Object constructed by Intl.DateTimeFormat must be extensible.");
}

