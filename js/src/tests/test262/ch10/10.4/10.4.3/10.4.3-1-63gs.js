










function f() { "use strict"; return this===undefined;};
if (! eval("f();")){
    throw "'this' had incorrect value!";
}