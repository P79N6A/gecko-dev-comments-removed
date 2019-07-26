


gc()
schedulegc(this)
gcslice(2)
function f() {
    this["x"] = this["x"] = {}
}
new f()
