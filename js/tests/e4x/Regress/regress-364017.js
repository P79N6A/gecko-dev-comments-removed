





































var bug = 364017;
var summary = 'Do not assert map->vector && i < map->length';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

if (typeof dis != 'undefined')
{
    dis( function() {
        XML.prototype.function::toString = function() { return "foo"; };
    });
}

TEST(1, expect, actual);

END();
