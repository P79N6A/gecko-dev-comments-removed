










function f() { "use strict"; return this===undefined;};
if (! f.call(undefined)){
    throw "'this' had incorrect value!";
}