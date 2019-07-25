



var s = evalcx("");
delete s.Object;
evalcx("var x;", s);

this.reportCompare(0, 0, "ok");
