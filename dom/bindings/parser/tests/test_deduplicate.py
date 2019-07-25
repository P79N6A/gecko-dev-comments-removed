import WebIDL

def WebIDLTest(parser, harness):
    parser.parse("""
        interface Foo;
        interface Bar;
        interface Foo;
        """);

    results = parser.finish()

    
    expectedNames = sorted(['Foo', 'Bar'])
    actualNames = sorted(map(lambda iface: iface.identifier.name, results))
    harness.check(actualNames, expectedNames, "Parser shouldn't output duplicate names.")
