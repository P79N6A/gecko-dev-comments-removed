










function f() { "use strict"; return this;};
if (f.apply(fnGlobalObject()) !== fnGlobalObject()){
    throw "'this' had incorrect value!";
}