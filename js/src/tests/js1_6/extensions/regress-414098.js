





var BUGNUMBER = 414098;
var summary = 'Getter behavior on arrays';
var actual = '';
var expect = '';

var a=[1,2,3];
var foo = 44;
a.__defineGetter__(1, function() { return foo + 10; });
actual = String(a);
reportCompare("1,54,3", actual, "getter 1");

actual = String(a.reverse());
reportCompare("3,54,1", actual, "reverse");

var s = "";
a.forEach(function(e) { s += e + "|"; });
actual = s;
reportCompare("3|54|1|", actual, "forEach");

actual = a.join(' - ');
reportCompare("3 - 54 - 1", actual, "join");

a[2]=3;
actual = a.pop();
reportCompare(actual, 3, "pop");

actual = a.pop();
reportCompare(actual, 54, "pop 2");
