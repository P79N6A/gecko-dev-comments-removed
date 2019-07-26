







testPassesUnlessItThrows();





function $ERROR(msg) {
    throw new Error("Test402 error: " + msg);
}





function $INCLUDE(file) {
    loadRelativeToScript("lib/" + file);
}




var __globalObject = Function("return this;")();
function fnGlobalObject() {
     return __globalObject;
}
