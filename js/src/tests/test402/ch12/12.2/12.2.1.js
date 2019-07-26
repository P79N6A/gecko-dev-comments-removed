







var desc = Object.getOwnPropertyDescriptor(Intl.DateTimeFormat, "prototype");
if (desc === undefined) {
    $ERROR("Intl.DateTimeFormat.prototype is not defined.");
}
if (desc.writable) {
    $ERROR("Intl.DateTimeFormat.prototype must not be writable.");
}
if (desc.enumerable) {
    $ERROR("Intl.DateTimeFormat.prototype must not be enumerable.");
}
if (desc.configurable) {
    $ERROR("Intl.DateTimeFormat.prototype must not be configurable.");
}

