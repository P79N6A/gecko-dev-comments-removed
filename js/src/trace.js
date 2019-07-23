f = function() {
	var q = 1;
	
	for (var i = 0; i < 5000; ++i)
		q += 2.5;
	print("q=" + q + " i=" + i);
}

var before = Date.now();
f();
var after = Date.now();
print(after - before);
