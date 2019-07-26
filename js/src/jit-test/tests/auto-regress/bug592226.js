




x = Proxy.create((function () {
    return {
        get: Object.create
    }
})([]), "")
try {
    (function () {
        for each(l in [0]) {
            print(x)
        }
    })()
} catch (e) {}
gc()
for each(let a in [0]) {
    print(x)
}
