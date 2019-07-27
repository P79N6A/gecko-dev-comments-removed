
var func = new Function("new.target");


assertThrowsInstanceOf(() => eval('new.target'), SyntaxError);

function evalInFunction() { eval('new.target'); }
assertThrowsInstanceOf(() => evalInFunction(), SyntaxError);

if (typeof reportCompare === "function")
    reportCompare(0,0,"OK");
