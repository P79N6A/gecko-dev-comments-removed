



if (typeof findReferences == "function") {

    var global = newGlobal('new-compartment');
    var o = ({});
    global.o = o;

    
    findReferences(o);

    reportCompare(true, true);

} else {
    reportCompare(true, true, "test skipped: findReferences is not a function");
}
