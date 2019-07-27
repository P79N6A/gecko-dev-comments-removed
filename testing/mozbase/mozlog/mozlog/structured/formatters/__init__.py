



import json
from unittest import UnittestFormatter
from xunit import XUnitFormatter
from html import HTMLFormatter
from machformatter import MachFormatter
from tbplformatter import TbplFormatter
from errorsummary import ErrorSummaryFormatter

def JSONFormatter():
    return lambda x: json.dumps(x) + "\n"
