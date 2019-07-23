





































import win32pdh
import win32pdhutil


class CounterManager:

  def __init__(self, process, counters=None):
    self.process = process
    self.registeredCounters = {}
    self.registerCounters(counters)
    
    
    win32pdh.EnumObjects(None, None, 0, 1)

  def registerCounters(self, counters):
    for counter in counters:
      self.registeredCounters[counter] = []
            
  def unregisterCounters(self, counters):
    for counter in counters:
      if counter in self.registeredCounters:
        del self.registeredCounters[counter]

  def getRegisteredCounters(self):
    return keys(self.registeredCounters)

  def getCounterValue(self, counter):
    hq = self.registeredCounters[counter][0]
    hc = self.registeredCounters[counter][1]
    try:
      win32pdh.CollectQueryData(hq)
      type, val = win32pdh.GetFormattedCounterValue(hc, win32pdh.PDH_FMT_LONG)
      return val
    except:
      return None

  def getProcess(self):
    return self.process

  def startMonitor(self):
    
    
    win32pdh.EnumObjects(None, None, 0, 1)

    for counter in self.registeredCounters:
      path = win32pdh.MakeCounterPath((None, 'process', self.process,
                                       None, -1, counter))
      hq = win32pdh.OpenQuery()
      try:
        hc = win32pdh.AddCounter(hq, path)
      except:
        win32pdh.CloseQuery(hq)

      self.registeredCounters[counter] = [hq, hc]

  def stopMonitor(self):
    for counter in self.registeredCounters:
      win32pdh.RemoveCounter(self.registeredCounters[counter][1])
      win32pdh.CloseQuery(self.registeredCounters[counter][0])
    self.registeredCounters.clear()
