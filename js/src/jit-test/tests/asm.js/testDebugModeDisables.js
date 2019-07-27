

load(libdir + "asm.js");


var g = newGlobal();
var dbg = new g.Debugger(this);

assertAsmTypeFail("'use asm'; function f() {} return f");
