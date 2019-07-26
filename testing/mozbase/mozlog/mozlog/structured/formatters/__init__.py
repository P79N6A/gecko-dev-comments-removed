



import json
from unittest import UnittestFormatter
from xunit import XUnitFormatter
from html import HTMLFormatter
from machformatter import MachFormatter, MachTerminalFormatter

def JSONFormatter():
    return lambda x: json.dumps(x) + "\n"
