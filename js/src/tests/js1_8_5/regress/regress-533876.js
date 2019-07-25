





var savedEval = eval;
var x = [0];
eval();

x.__proto__ = this;  
try {
    DIE;
} catch(e) {
}

delete eval;  
gc();
eval = savedEval;
var f = eval("(function () { return /x/; })");
x.watch('x', f);  

reportCompare("ok", "ok", "bug 533876");
