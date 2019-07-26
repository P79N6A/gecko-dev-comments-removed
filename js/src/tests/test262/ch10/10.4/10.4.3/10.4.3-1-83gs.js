










function f() {return this!==undefined;};
if (! ((function () {return Function("\"use strict\";return f();")();})()) ){
    throw "'this' had incorrect value!";
}
