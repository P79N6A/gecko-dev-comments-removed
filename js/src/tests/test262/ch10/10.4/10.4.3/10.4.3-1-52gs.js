










if (! ((function () {
    var f = function () {
        "use strict";
        return typeof this;
    }
    return (f()==="undefined") && (this===fnGlobalObject());
})())) {
    throw "'this' had incorrect value!";
}