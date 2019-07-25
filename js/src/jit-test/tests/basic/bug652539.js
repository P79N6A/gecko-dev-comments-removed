
options("strict", "werror");

try {
    throw 5;
} catch(e) {
    print(e + ',' + e.stack);
}
