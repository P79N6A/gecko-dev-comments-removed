










function f() { return this===fnGlobalObject();};
if (! ((function () {"use strict"; return f.bind(fnGlobalObject())();})())){
    throw "'this' had incorrect value!";
}
