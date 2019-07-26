


try {
    a = ArrayBuffer(76);
    b = Uint32Array(a);
    uneval()
    c = Uint8Array(a);
    c.set(b)
} catch (e) {}
