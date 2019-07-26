


function f(x) {
    this.i = x;
}
for each(let e in [0, 0, []]) {
    try {
        f(e)
    } catch (e) {}
}
print(uneval(this))
