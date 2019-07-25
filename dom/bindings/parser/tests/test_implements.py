
import WebIDL

def WebIDLTest(parser, harness):
    
    threw = False
    try:
        parser.parse("""
            A implements B;
            interface B {
              attribute long x;
            };
            interface A {
              attribute long y;
            };
        """)
        results = parser.finish()
    except:
        threw = True

    harness.ok(not threw, "Should not have thrown on implements statement "
               "before interfaces")
    harness.check(len(results), 3, "We have three statements")
    harness.ok(isinstance(results[1], WebIDL.IDLInterface), "B is an interface")
    harness.check(len(results[1].members), 1, "B has one member")
    A = results[2]
    harness.ok(isinstance(A, WebIDL.IDLInterface), "A is an interface")
    harness.check(len(A.members), 2, "A has two members")
    harness.check(A.members[0].identifier.name, "y", "First member is 'y'")
    harness.check(A.members[1].identifier.name, "x", "Second member is 'x'")

    
    threw = False
    try:
        parser.parse("""
            C implements D;
            interface D {
              attribute long x;
            };
            interface C {
              attribute long x;
            };
        """)
        parser.finish()
    except:
        threw = True

    harness.ok(threw, "Should have thrown on implemented interface duplicating "
               "a name on base interface")

    
    threw = False
    try:
        parser.parse("""
            E implements F;
            E implements G;
            interface F {
              attribute long x;
            };
            interface G {
              attribute long x;
            };
            interface E {};
        """)
        parser.finish()
    except:
        threw = True

    harness.ok(threw, "Should have thrown on implemented interfaces "
               "duplicating each other's member names")

    
    threw = False
    try:
        parser.parse("""
            H implements I;
            H implements J;
            I implements K;
            interface K {
              attribute long x;
            };
            interface L {
              attribute long x;
            };
            interface I {};
            interface J : L {};
            interface H {};
        """)
        parser.finish()
    except:
        threw = True

    harness.ok(threw, "Should have thrown on indirectly implemented interfaces "
               "duplicating each other's member names")

    
    threw = False
    try:
        parser.parse("""
            M implements N;
            interface O {
              attribute long x;
            };
            interface N : O {
              attribute long x;
            };
            interface M {};
        """)
        parser.finish()
    except:
        threw = True

    harness.ok(threw, "Should have thrown on implemented interface and its "
               "ancestor duplicating member names")

    
    
    parser = WebIDL.Parser()

    
    threw = False
    try:
        parser.parse("""
            P implements Q;
            P implements R;
            Q implements S;
            R implements S;
            interface Q {};
            interface R {};
            interface S {
              attribute long x;
            };
            interface P {};
        """)
        results = parser.finish()
    except:
        threw = True

    harness.ok(not threw, "Diamond inheritance is fine")
    harness.check(results[6].identifier.name, "S", "We should be looking at 'S'")
    harness.check(len(results[6].members), 1, "S should have one member")
    harness.check(results[6].members[0].identifier.name, "x",
                  "S's member should be 'x'")
