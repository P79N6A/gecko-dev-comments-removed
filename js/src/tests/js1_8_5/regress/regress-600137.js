





if (typeof evalcx == 'function') {
    var src = 'try {\n' +
    '    for (var [e] in d) {\n' +
    '        (function () {});\n' +
    '    }\n' +
    '} catch (e) {}\n' +
    'try {\n' +
    '    let(x = Object.freeze(this, /x/))\n' +
    '    e = {}.toString\n' +
    '    function y() {}\n' +
    '} catch (e) {}';

    evalcx(src);
}

reportCompare(0, 0, "don't crash");
