



if (typeof findReferences == "function") {

    var global = newGlobal();
    var o = ({});
    global.o = o;

    
    findReferences(o);

    reportCompare(true, true);

} else {
    reportCompare(true, true, "test skipped: findReferences is not a function");
}
