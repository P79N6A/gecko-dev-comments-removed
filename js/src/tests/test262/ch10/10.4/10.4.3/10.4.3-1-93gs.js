










var o = {};
function f() { return this===o;};
if (! ((function () {"use strict"; return f.call(o); })())){
    throw "'this' had incorrect value!";
}
