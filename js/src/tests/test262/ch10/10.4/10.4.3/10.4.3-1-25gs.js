










"use strict";
var obj = new (function () {
    return this;
});
if ((obj === fnGlobalObject()) || (typeof obj === "undefined")) {
    throw "'this' had incorrect value!";
}

