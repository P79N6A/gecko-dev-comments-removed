


function g(code) {
    try {
        f = eval("(function(){" + code + "})")
    } catch (r) {}
    f()
    try {
        evalcx("(function(){return" + code + "})()")
    } catch (e) {}
}
g("mjitChunkLimit(8)")
g(" function(x,[]){NaN.x::c}()")
