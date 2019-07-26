




(function () {
    x = Proxy.create((function () {
        return {
            enumerateOwn: function () Object.getOwnPropertyDescriptor
        }
    })(), [])
})()(uneval(this))
