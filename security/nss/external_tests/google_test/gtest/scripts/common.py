



























"""Shared utilities for writing scripts for Google Test/Mock."""

__author__ = 'wan@google.com (Zhanyong Wan)'


import os
import re







_SVN_INFO_URL_RE = re.compile(r'^URL: https://(\w+)\.googlecode\.com/svn(.*)')


def GetCommandOutput(command):
  """Runs the shell command and returns its stdout as a list of lines."""

  f = os.popen(command, 'r')
  lines = [line.strip() for line in f.readlines()]
  f.close()
  return lines


def GetSvnInfo():
  """Returns the project name and the current SVN workspace's root path."""

  for line in GetCommandOutput('svn info .'):
    m = _SVN_INFO_URL_RE.match(line)
    if m:
      project = m.group(1)  
      rel_path = m.group(2)
      root = os.path.realpath(rel_path.count('/') * '../')
      return project, root

  return None, None


def GetSvnTrunk():
  """Returns the current SVN workspace's trunk root path."""

  _, root = GetSvnInfo()
  return root + '/trunk' if root else None


def IsInGTestSvn():
  project, _ = GetSvnInfo()
  return project == 'googletest'


def IsInGMockSvn():
  project, _ = GetSvnInfo()
  return project == 'googlemock'
