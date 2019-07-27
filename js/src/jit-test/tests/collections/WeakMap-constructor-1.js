

load(libdir + "asserts.js");

new WeakMap();
new WeakMap(undefined);
new WeakMap(null);


assertWarning(() => WeakMap(), "None");



