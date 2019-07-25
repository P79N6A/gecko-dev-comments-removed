



for (let z = 0; z < 2; z++) {
    with({
        x: function() {}
    }) {
        for (l in [x]) {}
    }
}

reportCompare(0, 0, 'ok');
