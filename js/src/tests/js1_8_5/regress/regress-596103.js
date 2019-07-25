



for (var u = 0; u < 3; ++u) {
    var y = [];
    Object.create(y);
    gc();
    y.t = 3;
    gc();
}

reportCompare(0, 0, 'ok');
