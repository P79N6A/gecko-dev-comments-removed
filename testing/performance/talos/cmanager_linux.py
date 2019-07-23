






































"""A set of functions to run the Tp test.

   The Tp test measures page load times in Firefox.  It does this with a
   JavaScript script that opens a new window and cycles through page loads
   from the local disk, timing each one.  The script dumps the sum of the
   mean times to open each page, and the standard deviation, to standard out.
   We can also measure performance attributes during the test.  See below for
   what can be monitored
"""

__author__ = 'annie.sullivan@gmail.com (Annie Sullivan)'

import subprocess
import sys
import os
import time
import threading

import ffprocess

def GetPrivateBytes(pid):
  """Calculate the amount of private, writeable memory allocated to a process.
     This code was adapted from 'pmap.c', part of the procps project.
  """
  mapfile = '/proc/%s/maps' % pid
  maps = open(mapfile)

  private = 0

  for line in maps:
    
    (range,line) = line.split(" ", 1)

    (start,end) = range.split("-")
    flags = line.split(" ", 1)[0]

    size = int(end, 16) - int(start, 16)

    if flags.find("p") >= 0:
      if flags.find("w") >= 0:
        private += size

  return private


def GetResidentSize(pid):
  """Retrieve the current resident memory for a given process"""
  
  
  file = '/proc/%s/status' % pid

  status = open(file)

  for line in status:
    if line.find("VmRSS") >= 0:
      return int(line.split()[1]) * 1024


def GetCpuTime(pid, sampleTime=1):
  
  return 0

def GetXRes(pid):
  """Returns the total bytes used by X"""
  try: 
    cmdline = "xrestop -m 1 -b | grep -A 15 " + str(pid) + " | tr -d \"\n\" | sed \"s/.*total bytes.*: ~//g\""
    pipe = subprocess.Popen(cmdline, shell=True, stdout=-1).stdout
    data = pipe.read()
    pipe.close()
    return data
  except:
    print "Unexpected error:", sys.exc_info()
    return -1

counterDict = {}
counterDict["Private Bytes"] = GetPrivateBytes
counterDict["RSS"] = GetResidentSize
counterDict["% Processor Time"] = GetCpuTime
counterDict["XRes"] = GetXRes

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
      os.stat('/proc/%s' % self.pid)
      self.runThread = True
      self.start()
    except:
      print 'WARNING: problem starting counter monitor'

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
