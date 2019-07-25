




function make_watcher(name) {
    return function (id, oldv, newv) {
        print("watched " + name + "[0]");
    };
}

var o, p;
function f(flag) {
    if (flag) {
        o = arguments;
    } else {
        p = arguments;
        o.watch(0, make_watcher('o'));
        p.watch(0, make_watcher('p'));

        

















        
        p.unwatch(0);

        
        o.unwatch(0);

        
        p[0] = 4;

        
        assertEq(flag, 4);
    }
}

f(true);
f(false);

reportCompare(true, true);
