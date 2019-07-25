



i = 42
eval("let(y){(function(){let({}=y){(function(){let({}=y=[])(i)})()}})()}")
reportCompare(0, 0, "ok");
