

(function () {
    for (b in [0, 0]) {
        (eval("\
            [this\
                for (b in [\
                    [undefined],\
                    arguments,\
                    [undefined]\
                ])\
            ]\
        "))
    }
})()
