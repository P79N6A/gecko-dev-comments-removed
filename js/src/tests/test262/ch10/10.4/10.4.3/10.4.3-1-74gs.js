










var o = {};
function f() { "use strict"; return this===o;};
if (! f.call(o)){
    throw "'this' had incorrect value!";
}