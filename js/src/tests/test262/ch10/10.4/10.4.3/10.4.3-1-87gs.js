










function f() { return this===fnGlobalObject();};
if (! ((function () {"use strict"; return f.apply(undefined);})())){
    throw "'this' had incorrect value!";
}