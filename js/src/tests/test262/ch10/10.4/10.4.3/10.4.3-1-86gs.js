










function f() { return this===fnGlobalObject();};
if (! ((function () {"use strict"; return f.apply(null);})())){
    throw "'this' had incorrect value!";
}
