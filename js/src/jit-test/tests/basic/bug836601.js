
let k
Proxy.createFunction(function() {
    return {
        get: (n) => new Uint32Array(n)
    }
}(), decodeURIComponent) & k

