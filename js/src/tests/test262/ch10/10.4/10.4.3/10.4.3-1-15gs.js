










"use strict";
var f = new Function("return typeof this;");
if (f() === "undefined") {
    throw "'this' had incorrect value!";
}
