



import unittest

from mozunit import main

from mozbuild.frontend.sandbox_symbols import (
    FUNCTIONS,
    SPECIAL_VARIABLES,
    VARIABLES,
)


class TestSymbols(unittest.TestCase):
    def _verify_doc(self, doc):
        
        
        
        
        

        self.assertNotIn('\r', doc)

        lines = doc.split('\n')

        
        for line in lines[0:-1]:
            self.assertEqual(line, line.rstrip())

        self.assertGreater(len(lines), 0)
        self.assertGreater(len(lines[0].strip()), 0)

        
        self.assertEqual(lines[-1].strip(), '')

    def test_documentation_formatting(self):
        for typ, default, doc in VARIABLES.values():
            self._verify_doc(doc)

        for attr, args, doc in FUNCTIONS.values():
            self._verify_doc(doc)

        for typ, doc in SPECIAL_VARIABLES.values():
            self._verify_doc(doc)


if __name__ == '__main__':
    main()
