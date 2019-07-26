







var desc = Object.getOwnPropertyDescriptor(Intl.NumberFormat, "prototype");
if (desc === undefined) {
    $ERROR("Intl.NumberFormat.prototype is not defined.");
}
if (desc.writable) {
    $ERROR("Intl.NumberFormat.prototype must not be writable.");
}
if (desc.enumerable) {
    $ERROR("Intl.NumberFormat.prototype must not be enumerable.");
}
if (desc.configurable) {
    $ERROR("Intl.NumberFormat.prototype must not be configurable.");
}

