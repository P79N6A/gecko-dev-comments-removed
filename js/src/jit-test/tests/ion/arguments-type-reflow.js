


var forceReflow = false;

function rec(x, self) {
    if (x > 0)
        self(x - 1, self);
    else if (forceReflow)
        self(NaN, self);
}

for (var i = 0; i < 40; ++i)
    rec(1, rec);

forceReflow = true;
rec(1, rec);
