


var sandbox = evalcx("");
function e(code) { try { evalcx(code, sandbox); } catch(e) { } }
e("let x;");
e("Object.seal(this);");
e("x=Proxy.createFunction({keys:Object.getPrototypeOf},function(){})");
e("const y;");

