










function f() { "use strict"; return this===null;};
if (! (f.bind(null)())){
    throw "'this' had incorrect value!";
}