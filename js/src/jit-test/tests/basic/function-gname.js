function bytecode(f) {
    if (typeof disassemble !== "function")
        return "unavailable";
    var d = disassemble(f);
    return d.slice(d.indexOf("main:"), d.indexOf("\n\n"));
}

function hasGname(f, v) {
    
    
    try {
	var b = bytecode(f);
	if (b != "unavailable") {
	    assertEq(b.contains(`getgname "${v}"`), true);
	    assertEq(b.contains(`getname "${v}"`), false);
	}
    } catch (e) {
	print(e.stack);
	throw e;
    }
}

var x = "outer";

var f1 = new Function("assertEq(x, 'outer')");
f1();
hasGname(f1, 'x');

setLazyParsingEnabled(false);
var f2 = new Function("assertEq(x, 'outer')");
f2();
hasGname(f2, 'x');
setLazyParsingEnabled(true);

{
    let x = "inner";
    var f3 = new Function("assertEq(x, 'outer')");
    f3();
    hasGname(f3, 'x');
}

setLazyParsingEnabled(false);
{
    let x = "inner";
    var f4 = new Function("assertEq(x, 'outer')");
    f4();
    hasGname(f4, 'x');
}
setLazyParsingEnabled(true);
