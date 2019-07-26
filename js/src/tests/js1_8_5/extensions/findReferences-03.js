



if (typeof findReferences == "function") {

    function makeGenerator(c) { eval(c); yield function generatorClosure() { return x; }; }
    var generator = makeGenerator('var x = 42');
    var closure = generator.next();
    referencesVia(closure, 'parent; generator object', generator);

    var o = {};

    assertEq(function f() {
	return referencesVia(null, 'arguments', arguments) ||
	       referencesVia(null, 'baseline-args-obj', arguments);
    }(), true);

    var rvalueCorrect;

    function finallyHoldsRval() {
        try {
            return o;
        } finally {
            rvalueCorrect = referencesVia(null, 'rval', o);
        }
    }
    rvalueCorrect = false;
    finallyHoldsRval();
    assertEq(rvalueCorrect, true);

    
    
    
    
    
    
    
    

    reportCompare(true, true);
} else {
    reportCompare(true, true, "test skipped: findReferences is not a function");
}
