

var count = 0;
this.watch("x", function() {
    count++;
});
for(var i=0; i<10; i++) {
    x = 2;
}
assertEq(count, 10);
