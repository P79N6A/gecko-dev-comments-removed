function s(f) { this._m = f; }

function C(i) {
    Object.defineProperty(this, "m", {set: s});
    this.m = function () { return 17; };
}

var arr = [];
for (var i = 0; i < 9; i++)
    arr[i] = new C(i);

checkStats({recorderStarted: 1, recorderAborted: 0, traceCompleted: 1, traceTriggered: 1});




