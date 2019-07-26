










if (typeof Intl.NumberFormat.prototype.format(0) !== "string") {
    $ERROR("Intl.NumberFormat's prototype is not an object that has been " +
        "initialized as an Intl.NumberFormat");
}

