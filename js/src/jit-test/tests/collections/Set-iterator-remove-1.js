

function test(letters, toRemove) {
    var set = Set(letters);
    toRemove = Set(toRemove);

    var leftovers = [x for (x of set) if (!toRemove.has(x))].join("");

    var log = "";
    for (let x of set) {
        log += x;
        if (toRemove.has(x))
            set.delete(x);
    }
    assertEq(log, letters);

    var remaining = [x for (x of set)].join("");
    assertEq(remaining, leftovers);
}

test('a', 'a');    
test('abc', 'a');  
test('abc', 'b');  
test('abc', 'c');  
test('abc', 'abc') 

