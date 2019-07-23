



































"""Utility functions"""

import os
DEBUG = 0
saved_environment = {}

def setdebug(val):
  global DEBUG
  DEBUG = val

def debug(message):
  """Prints a debug message to the console if the DEBUG switch is turned on 
     debug switch is controlled through command line switch (-d or --debug)
     Args:
       message: string containing a debugging statement
     """
  if DEBUG == 1:
    print message

def setEnvironmentVars(newVars): 
   """Sets environment variables as specified by env, an array of variables 
   from sample.config"""
   global saved_environment
   env = os.environ
   for var in newVars:
     
     try:
       saved_environment[var] = str(env[var])
     except :
       saved_environment[var] = ""
     env[var] = str(newVars[var])

def restoreEnvironmentVars():
  """Restores environment variables to the state they were in before 
  setEnvironmentVars() was last called"""
  global saved_environment
  for var in saved_environment:
    os.environ[var] = saved_environment[var]
