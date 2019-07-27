




var opts = new Proxy({loc: false}, {});
assertEq(Reflect.parse("0;", opts).loc === null, true);
opts.loc = true;
assertEq(Reflect.parse("0;", opts).loc !== null, true);
delete opts.loc;
assertEq(Reflect.parse("0;", opts).loc !== null, true);  


var builder = {
    program: function (body) { return body.join(); },
    expressionStatement: function (expr) { return expr + ";" },
    literal: function (val) { return "" + val; }
};
opts = {builder: new Proxy(builder, {})};
assertEq(Reflect.parse("0xff;", opts), "255;");

reportCompare(0, 0, 'ok');
