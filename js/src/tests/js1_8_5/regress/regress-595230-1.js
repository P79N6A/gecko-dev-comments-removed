



var box = evalcx('lazy');

var src =
    'try {\n' +
    '    __proto__ = Proxy.createFunction((function() {}), function() {})\n' +
    '    var x\n' +
    '    *\n' +
    '} catch(e) {}\n' +
    'default xml namespace = x\n' +
    'for (let b in [0, 0]) <x/>\n' +
    '0\n';

evalcx(src, box);

this.reportCompare(0, 0, "ok");
