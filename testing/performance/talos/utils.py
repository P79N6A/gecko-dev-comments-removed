



































"""Utility functions"""

import config
import os

saved_environment = {}

def debug(message):
  """Prints a debug message to the console if the DEBUG switch is turned on 
     in config.py
     Args:
       message: string containing a debugging statement
     """
  if config.DEBUG == 1:
    print message

def setEnvironmentVars(newVars): 
   """Sets environment variables as specified by env, an array of variables 
   from config.py"""
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
  for var in saved_environment:
    os.environ[var] = saved_environment[var]
