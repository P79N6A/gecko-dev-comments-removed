
""" This script is an example of why I care so much about Mozharness' 2nd core
concept, logging.  http://escapewindow.dreamwidth.org/230853.html
"""

import os
import shutil


os.system("curl -s -o foo.tar.bz2 http://people.mozilla.org/~asasaki/foo.tar.bz2")



os.system("tar xjf foo.tar.bz2")


os.remove("x/ship2")
os.remove("foo.tar.bz2")
os.system("tar cjf foo.tar.bz2 x")
shutil.rmtree("x")

os.remove("foo.tar.bz2")
