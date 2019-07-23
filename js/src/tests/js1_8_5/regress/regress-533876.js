




gTestfile = 'regress-533876';

var x = [0];
eval();

x.__proto__ = this;  
try {
    DIE;
} catch(e) {
}

delete eval;  
gc();
var f = eval("function () { return /x/; }");
x.watch('x', f);  
print(" PASSED!");
