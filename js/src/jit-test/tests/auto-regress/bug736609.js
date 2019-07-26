


try {
    a = new ArrayBuffer(76);
    b = new Uint32Array(a);
    uneval()
    c = new Uint8Array(a);
    c.set(b)
} catch (e) {}
