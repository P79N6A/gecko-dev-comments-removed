
var ast = Reflect.parse(toLatin1("function f() { return 3; }"));
assertEq(ast.body[0].id.name, "f");


var ast = Reflect.parse("function f\u1200() { return 3; }");
assertEq(ast.body[0].id.name, "f\u1200");
