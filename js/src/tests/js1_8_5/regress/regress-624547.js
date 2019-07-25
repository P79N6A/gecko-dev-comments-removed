




function f(x) {
    delete arguments[0];
    for(var i=0; i<20; i++) {
        arguments[0] !== undefined;
    }
}


f(1);

reportCompare(0, 0, "ok");
