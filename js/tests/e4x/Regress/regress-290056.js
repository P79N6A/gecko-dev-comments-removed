




































gTestfile = 'regress-290056.js';

var summary = 'Dont crash when serializing an XML object where the name ' +
    'of an attribute was changed with setName';

var BUGNUMBER = 290056;
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

var link = <link type="simple" />;
var xlinkNamespace = new Namespace('xlink', 'http://www.w3.org/1999/xlink');
link.addNamespace(xlinkNamespace);
printStatus('In scope namespace: ' + link.inScopeNamespaces());
printStatus('XML markup: ' + link.toXMLString());
link.@type.setName(new QName(xlinkNamespace.uri, 'type'));
printStatus('name(): ' + link.@*::*[0].name());
printStatus(link.toXMLString());

TEST(1, expect, actual);

END();
