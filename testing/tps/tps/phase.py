




































import os
import re

class TPSTestPhase(object):

  lineRe = re.compile(
      r"^(.*?)test phase (?P<matchphase>\d+): (?P<matchstatus>.*)$")

  def __init__(self, phase, profile, testname, testpath, logfile, env,
               firefoxRunner, logfn):
    self.phase = phase
    self.profile = profile
    self.testname = str(testname) 
    self.testpath = testpath
    self.logfile = logfile
    self.env = env
    self.firefoxRunner = firefoxRunner
    self.log = logfn
    self._status = None
    self.errline = ''

  @property
  def phasenum(self):
    match = re.match('.*?(\d+)', self.phase)
    if match:
      return match.group(1)

  @property
  def status(self):
    return self._status if self._status else 'unknown'

  def run(self):
    
    args = [ '-tps', self.testpath, 
             '-tpsphase', self.phasenum, 
             '-tpslogfile', self.logfile ]

    self.log("\nlaunching firefox for phase %s with args %s\n" % 
             (self.phase, str(args)))
    returncode = self.firefoxRunner.run(env=self.env,
                                        args=args, 
                                        profile=self.profile)

    
    found_test = False
    f = open(self.logfile, 'r')
    for line in f:

      
      if not found_test:
        if line.find("Running test %s" % self.testname) > -1:
          found_test = True
        else:
          continue

      
      match = self.lineRe.match(line)
      if match:
        if match.group("matchphase") == self.phasenum:
          self._status = match.group("matchstatus")
          break

      
      if line.find("CROSSWEAVE ERROR: ") > -1 and not self._status:
        self._status = "FAIL"
        self.errline = line[line.find("CROSSWEAVE ERROR: ") + len("CROSSWEAVE ERROR: "):]

    f.close()

