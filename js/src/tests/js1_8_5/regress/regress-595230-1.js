




var box = evalcx('lazy');

var src =
    'try {\n' +
    '    __proto__ = Proxy.createFunction((function() {}), function() {})\n' +
    '    var x\n' +
    '    {}\n' +
    '} catch(e) {}\n' +
    'for (let b in [0, 0]) {}\n' +
    '0\n';

evalcx(src, box);

this.reportCompare(0, 0, "ok");
