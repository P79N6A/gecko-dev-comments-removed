


__defineSetter__("x", function(){})
this.watch("x", "".localeCompare)
window = x
Object.defineProperty(this, "x", ({
    set: window
}))
