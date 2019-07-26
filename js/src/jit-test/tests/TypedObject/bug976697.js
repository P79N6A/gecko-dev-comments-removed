


if (!this.hasOwnProperty("TypedObject"))
  quit();

x = ArrayBuffer();
neuter(x);
Uint32Array(x);
gc();
