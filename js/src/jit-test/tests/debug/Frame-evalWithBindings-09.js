
var g = newGlobal('new-compartment');
var dbg = new Debug(g);
var f1;
var hits = 0;
dbg.hooks = {
    debuggerHandler: function (frame) {
	f1 = frame;

	
	var x = frame.evalWithBindings("wrongSpeling", {rightSpelling: 2}).throw;

	assertEq(frame.evalWithBindings("exc.name", {exc: x}).return, "ReferenceError");
	hits++;
    },
    throw: function (frame, exc) {
	assertEq(frame !== f1, true);

	
	assertEq(f1.eval("rightSpelling").return, "dependent");
	assertEq(f1.evalWithBindings("n + rightSpelling", {n: "in"}).return, "independent");

	
	assertEq(frame.eval("rightSpelling").return, 2);
	assertEq(frame.evalWithBindings("rightSpelling + three", {three: 3}).return, 5);
	hits++;
    }
};
g.eval("(function () { var rightSpelling = 'dependent'; debugger; })();");
