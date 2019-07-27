





x = new ArrayBuffer();
neuter(x, "same-data");
new Uint32Array(x);
gc();

x = new ArrayBuffer();
neuter(x, "change-data");
new Uint32Array(x);
gc();
