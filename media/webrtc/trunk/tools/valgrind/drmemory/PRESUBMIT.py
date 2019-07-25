



"""
See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details on the presubmit API built into gcl.
"""

import os
import re
import sys

def CheckChange(input_api, output_api):
  """Checks the DrMemory suppression files for bad suppressions."""

  
  
  tools_vg_path = os.path.join(input_api.PresubmitLocalPath(), '..')
  sys.path.append(tools_vg_path)
  import suppressions

  return suppressions.PresubmitCheck(input_api, output_api)

def CheckChangeOnUpload(input_api, output_api):
  return CheckChange(input_api, output_api)

def CheckChangeOnCommit(input_api, output_api):
  return CheckChange(input_api, output_api)

def GetPreferredTrySlaves():
  return ['win_drmemory']
