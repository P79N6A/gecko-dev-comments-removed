







































"""A set of functions to run the Tp test.

   The Tp test measures page load times in Firefox.  It does this with a
   JavaScript script that opens a new window and cycles through page loads
   from the local disk, timing each one.  The script dumps the sum of the
   mean times to open each page, and the standard deviation, to standard out.
   We can also measure performance attributes during the test.  See below for
   what can be monitored
"""

__author__ = 'annie.sullivan@gmail.com (Annie Sullivan)'


import os
import time
import threading
import subprocess

import ffprocess

def GetProcessData(pid):
  """Runs a ps on the process identified by pid and returns the output line
    as a list (uid, pid, ppid, cpu, pri, ni, vsz, rss, wchan, stat, tt, time, command)
  """
  command = ['ps -Acup'+str(pid)]
  handle = subprocess.Popen(command, stdout=subprocess.PIPE, universal_newlines=True, shell=True)
  handle.wait()
  data = handle.stdout.readlines()
  
  
  for line in data:
    if line.find(str(pid)) >= 0:
      
      line = line.split()
      if (line[1] == str(pid)):
        return line

def GetPrivateBytes(pid):
  """Calculate the amount of private, writeable memory allocated to a process.
  """
  psData = GetProcessData(pid)
  return psData[5]


def GetResidentSize(pid):
  """Retrieve the current resident memory for a given process"""
  psData = GetProcessData(pid)
  return psData[4]

def GetCpuTime(pid):
  
  return 0

counterDict = {}
counterDict["Private Bytes"] = GetPrivateBytes
counterDict["RSS"] = GetResidentSize
counterDict["% Processor Time"] = GetCpuTime

class CounterManager(threading.Thread):
  """This class manages the monitoring of a process with any number of
     counters.

     A counter can be any function that takes an argument of one pid and
     returns a piece of data about that process.
     Some examples are: CalcCPUTime, GetResidentSize, and GetPrivateBytes
  """
  
  pollInterval = .25

  def __init__(self, process, counters=None):
    """Args:
         counters: A list of counters to monitor. Any counters whose name does
         not match a key in 'counterDict' will be ignored.
    """
    self.allCounters = {}
    self.registeredCounters = {}
    self.process = process
    self.runThread = False
    self.pid = -1

    self._loadCounters()
    self.registerCounters(counters)

    threading.Thread.__init__(self)

  def _loadCounters(self):
    """Loads all of the counters defined in the counterDict"""
    for counter in counterDict.keys():
      self.allCounters[counter] = counterDict[counter]

  def registerCounters(self, counters):
    """Registers a list of counters that will be monitoring.
       Only counters whose names are found in allCounters will be added
    """
    for counter in counters:
      if counter in self.allCounters:
        self.registeredCounters[counter] = \
          [self.allCounters[counter], []]

  def unregisterCounters(self, counters):
    """Unregister a list of counters.
       Only counters whose names are found in registeredCounters will be
       paid attention to
    """
    for counter in counters:
      if counter in self.registeredCounters:
        del self.registeredCounters[counter]

  def getRegisteredCounters(self):
    """Returns a list of the registered counters."""
    return keys(self.registeredCounters)

  def getCounterValue(self, counterName):
    """Returns the last value of the counter 'counterName'"""
    try:
      if counterName is "% Processor Time":
        return self._getCounterAverage(counterName)
      else:
        return self.registeredCounters[counterName][1][-1]
    except:
      return None

  def _getCounterAverage(self, counterName):
    """Returns the average value of the counter 'counterName'"""
    try:
      total = 0
      for v in self.registeredCounters[counterName][1]:
        total += v
      return total / len(self.registeredCounters[counterName][1])
    except:
      return None

  def getProcess(self):
    """Returns the process currently associated with this CounterManager"""
    return self.process

  def startMonitor(self):
    """Starts the monitoring process.
       Throws an exception if any error occurs
    """
    
    try:
      
      self.pid = ffprocess.GetPidsByName(self.process)[-1]
      self.runThread = True
      self.start()
    except:
      raise

  def stopMonitor(self):
    """Stops the monitor"""
    
    
    self.runThread = False

  def run(self):
    """Performs the actual monitoring of the process. Will keep running
       until stopMonitor() is called
    """
    while self.runThread:
      for counter in self.registeredCounters.keys():
        
        
        
        try:
          self.registeredCounters[counter][1].append(
            self.registeredCounters[counter][0](self.pid))
        except:
          
          self.unregisterCounters([counter])

      time.sleep(self.pollInterval)
