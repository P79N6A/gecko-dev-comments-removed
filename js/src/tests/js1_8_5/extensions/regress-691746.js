


var obj = {};
try {
    obj.watch(QName(), function () {});
} catch (exc) {
}
gc();

reportCompare(0, 0, 'ok');
