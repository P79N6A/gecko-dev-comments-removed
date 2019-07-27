



def should_throw(parser, harness, message, code):
    parser = parser.reset();
    threw = False
    try:
        parser.parse(code)
        parser.finish()
    except:
        threw = True

    harness.ok(threw, "Should have thrown: %s" % message)


def WebIDLTest(parser, harness):
    
    should_throw(parser, harness, "no arguments", """
        interface I {
          [Replaceable=X] readonly attribute long A;
        };
    """)

    
    
    should_throw(parser, harness, "PutForwards", """
        interface I {
          [PutForwards=B, Replaceable] readonly attribute J A;
        };
        interface J {
          attribute long B;
        };
    """)

    
    
    should_throw(parser, harness, "writable attribute", """
        interface I {
          [Replaceable] attribute long A;
        };
    """)

    
    
    should_throw(parser, harness, "static attribute", """
        interface I {
          [Replaceable] static readonly attribute long A;
        };
    """)

    
    
    should_throw(parser, harness, "callback interface", """
        callback interface I {
          [Replaceable] readonly attribute long A;
        };
    """)
