




































"""Test the xpcom.file module."""
from pyxpcom_test_tools import suite_from_functions, testmain

import xpcom.file


def suite():
    return suite_from_functions(xpcom.file._TestAll)

if __name__=='__main__':
    testmain()

