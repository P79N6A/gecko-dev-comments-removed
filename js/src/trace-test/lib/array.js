




function Elements(arrayLike) {
    this.i = 0;
    this.stop = arrayLike.length;
    this.source = arrayLike;
}
Elements.prototype = {
    
    __iterator__: function() this,
    next: function() {
        if (this.i >= this.stop)
            throw StopIteration;
        return this.source[this.i++];
    }
};
function elements(arrayLike) {
    return new Elements(arrayLike);
}
