function f(b) {
	var z = 0;

	while (b < 10000000) {
		b++;
		
		
		if (b < 0) return;
		z = b*2;
		z = z*2;
	}
	return z;
}

for (var i = 0; i < 1200; i++) {
	f(i);
}
