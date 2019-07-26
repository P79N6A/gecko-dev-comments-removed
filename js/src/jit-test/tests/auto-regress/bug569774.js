


x = Proxy.create((function () {}), (evalcx('')))
try {
    (function () {
        ((let(e = eval) e).call(x, ("\"\"")))
    })()
} catch (e) {}
