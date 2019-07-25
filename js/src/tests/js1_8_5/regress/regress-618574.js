





var x = Proxy.create({
    iterate: function () {
            function f(){}
            f.next = function () { throw StopIteration; }
            return f;
        }
    });

for each (var e in [{}, {}, {}, {}, {}, {}, {}, {}, x]) {
    for (var v in e)  
        ;
}

reportCompare(0, 0, 'ok');
