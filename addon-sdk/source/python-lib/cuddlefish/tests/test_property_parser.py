



import unittest

from cuddlefish.property_parser import parse, MalformedLocaleFileError

class TestParser(unittest.TestCase):

    def test_parse(self):
        lines = [
          
          "sharp=#can be in value",
          "# comment",
          "#key=value",
          "  # comment2",

          "keyWithNoValue=",
          "valueWithSpaces=   ",
          "valueWithMultilineSpaces=  \\",
          "  \\",
          "  ",

          
          " key = value ",
          "key2=value2",
          
          "%s key=%s value",

          
          "",
          "   ",

          
          "multi=line\\", "value",
          
          "some= spaces\\", " are\\  ", " stripped ",
          
          "but=not \\", "all of \\", " them ",

          
          "explicitPlural[one] = one",
          "explicitPlural[other] = other",

          
          "implicitPlural[one] = one",
          "implicitPlural = other", 
        ]
        
        
        lines = [unicode(l + "\n") for l in lines]
        pairs = parse(lines)
        expected = {
          "sharp": "#can be in value",

          "key": "value",
          "key2": "value2",
          "%s key": "%s value",

          "keyWithNoValue": "",
          "valueWithSpaces": "",
          "valueWithMultilineSpaces": "",

          "multi": "linevalue",
          "some": "spacesarestripped",
          "but": "not all of them",

          "implicitPlural": {
            "one": "one",
            "other": "other"
          },
          "explicitPlural": {
            "one": "one",
            "other": "other"
          },
        }
        self.assertEqual(pairs, expected)

    def test_exceptions(self):
        self.failUnlessRaises(MalformedLocaleFileError, parse,
                              ["invalid line with no key value"])
        self.failUnlessRaises(MalformedLocaleFileError, parse,
                              ["plural[one]=plural with no [other] value"])
        self.failUnlessRaises(MalformedLocaleFileError, parse,
                              ["multiline with no last empty line=\\"])
        self.failUnlessRaises(MalformedLocaleFileError, parse,
                              ["=no key"])
        self.failUnlessRaises(MalformedLocaleFileError, parse,
                              ["   =only spaces in key"])

if __name__ == "__main__":
    unittest.main()
