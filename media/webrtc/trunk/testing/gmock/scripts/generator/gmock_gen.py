















"""Driver for starting up Google Mock class generator."""

__author__ = 'nnorwitz@google.com (Neal Norwitz)'

import os
import sys

if __name__ == '__main__':
  
  sys.path.append(os.path.dirname(__file__))

  from cpp import gmock_class
  
  gmock_class.__doc__ = gmock_class.__doc__.replace('gmock_class.py', __file__)
  gmock_class.main()
