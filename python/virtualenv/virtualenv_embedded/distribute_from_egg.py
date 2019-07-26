


import sys
eggname = sys.argv[-1]
sys.path.insert(0, eggname)
from setuptools.command.easy_install import main
main(sys.argv[1:])
