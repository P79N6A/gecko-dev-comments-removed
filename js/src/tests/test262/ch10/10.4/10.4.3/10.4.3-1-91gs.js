










function f() { return this===fnGlobalObject();};
if (! ((function () {"use strict"; return f.call(null); })())){
    throw "'this' had incorrect value!";
}
