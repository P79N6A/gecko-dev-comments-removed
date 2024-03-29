

load(libdir + "iteration.js");

function *f1() {
    yield 1;
    yield 2;
}


var it = f1();
assertIteratorResult(it.return(3), 3, true);
assertIteratorResult(it.return(Math), Math, true);
assertIteratorResult(it.return(), undefined, true);
assertIteratorDone(it);


it = f1();
assertIteratorNext(it, 1);
assertIteratorResult(it.return(null), null, true);
assertIteratorDone(it);


function *f2() {
    try {
        yield 1;
        yield 2;
    } finally {
        return 9;
    }
}
it = f2();
assertIteratorNext(it, 1);
assertIteratorResult(it.return(3), 9, true);
assertIteratorDone(it);



function *f3() {
    try {
        try {
            yield 1;
            yield 2;
        } finally {
            yield 3;
        }
    } finally {
        yield 4;
    }
}
it = f3();
assertIteratorNext(it, 1);
assertIteratorResult(it.return(9), 3, false);
assertIteratorNext(it, 4);
assertIteratorDone(it, 9);
assertIteratorDone(it, undefined);


function *f4() {
    try {
        yield 1;
        yield 2;
    } finally {
        throw 3;
    }
}
it = f4();
assertIteratorNext(it, 1);
assertThrowsValue(() => it.return(8), 3);
assertIteratorDone(it);

function *f5() {}
it = f5();
assertIteratorDone(it);
assertIteratorResult(it.return(3), 3, true);
assertIteratorDone(it);

function *f6() {
    try {
        yield 1;
        yield 2;
    } finally {
        try {
            return 9;
        } finally {
            yield 3;
        }
    }
}
it = f6();
assertIteratorNext(it, 1);
assertIteratorResult(it.return(5), 3, false);
assertIteratorDone(it, 9);
assertIteratorDone(it);



function *f7() {
    try {
        yield 1;
        yield 2;
    } finally {
        try {
            yield 3;
        } finally {
            yield 4;
        }
    }
}
it = f7();
assertIteratorNext(it, 1);
assertIteratorResult(it.return(5), 3, false);
assertIteratorResult(it.return(6), 4, false);
assertIteratorDone(it, 6);
assertIteratorDone(it);


function *f8() {
    try {
        yield 1;
        yield 2;
    } finally {
        yield 3;
    }
}
it = f8();
assertIteratorNext(it, 1);
assertIteratorResult(it.return(5), 3, false);
assertThrowsValue(() => it.throw(4), 4);
assertIteratorDone(it);


function *f9() {
    try {
        yield 1;
        yield 2;
    } finally {
        it.return(4);
        yield 3;
    }
}
it = f9();
assertIteratorNext(it, 1);
assertThrowsInstanceOf(() => it.return(5), TypeError);
assertIteratorDone(it);
assertIteratorDone(it);


function *f10() {
    try {
        yield 1;
    } finally {
        yield 2;
    }
}
it = f10();
assertIteratorNext(it, 1);
assertIteratorResult(it.return(-1), 2, false);
assertIteratorResult(it.return(-2), -2, true);
assertIteratorDone(it);

function *f11() {
    try {
        try {
            yield 1;
        } finally {
            throw 2;
        }
    } catch(e) {
        yield e;
    } finally {
        yield 3;
    }
}
it = f11();
assertIteratorNext(it, 1);
assertIteratorResult(it.return(9), 2, false);
assertIteratorNext(it, 3);
assertIteratorDone(it);

throw "done";
