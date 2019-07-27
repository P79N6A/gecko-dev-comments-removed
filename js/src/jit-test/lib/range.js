





function Range(start, stop) {
    this.i = start;
    this.stop = stop;
}
Range.prototype = {
    __iterator__: function() { return this; },
    next: function() {
        if (this.i >= this.stop)
            throw StopIteration;
        return this.i++;
    }
};

function range(start, stop) {
    return new Range(start, stop);
}
