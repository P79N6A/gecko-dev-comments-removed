

load(libdir + "asm.js");

assertAsmTypeFail("'use asm'; function f() {} return f");
