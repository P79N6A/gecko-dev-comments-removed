


function foo(f) {
    f()
}
foo((eval("\
    (function () {\
        for each(l in [0, 0xB504F332, 0]) {\
            for (d in Error()) {}\
        }\
    })\
")))
