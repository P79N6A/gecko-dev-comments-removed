


(function () {
    x = Proxy.createFunction((function () {
        return {
            getOwnPropertyDescriptor: function () {
                return this
            },
            get: undefined
        }
    })(), Object.getOwnPropertyDescriptor)
})()
x(x)
