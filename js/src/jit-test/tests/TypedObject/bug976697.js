





x = ArrayBuffer();
neuter(x, "same-data");
Uint32Array(x);
gc();

x = ArrayBuffer();
neuter(x, "change-data");
Uint32Array(x);
gc();
