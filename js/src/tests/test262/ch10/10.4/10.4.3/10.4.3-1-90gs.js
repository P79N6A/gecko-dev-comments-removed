










function f() { return this===fnGlobalObject();};
if (! ((function () {"use strict"; return f.call();})())){
    throw "'this' had incorrect value!";
}
