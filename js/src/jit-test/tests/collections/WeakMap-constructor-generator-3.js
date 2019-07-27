

new WeakMap(x for (x of []));

function none() {
    if (0) yield 0;
}
new WeakMap(none());

function* none2() {
    if (0) yield 0;
}
new WeakMap(none2());
