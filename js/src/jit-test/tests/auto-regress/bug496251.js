


let (z = {}) {
    for (var i = 0; i < 4; ++i) {
        for each (var e in [{}, 1, {}]) {
            +(function () z)();
        }
    }
}
