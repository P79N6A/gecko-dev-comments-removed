







var obj = new Intl.Collator();

var actualPrototype = Object.getPrototypeOf(obj);
if (actualPrototype !== Intl.Collator.prototype) {
    $ERROR("Prototype of object constructed by Intl.Collator isn't Intl.Collator.prototype; got " + actualPrototype);
}

if (!Object.isExtensible(obj)) {
    $ERROR("Object constructed by Intl.Collator must be extensible.");
}

