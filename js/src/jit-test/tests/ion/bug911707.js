
x = [ "CNY", "TWD", "invalid" ];
Object.freeze(x).map(function() {
    x.length = 6
})
