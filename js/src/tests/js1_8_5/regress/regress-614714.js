


function f(reportCompare) {
    if (typeof clear === 'function')
        clear(this);
    return f;
}


reportCompare(0, 0, 'ok');
f();  
