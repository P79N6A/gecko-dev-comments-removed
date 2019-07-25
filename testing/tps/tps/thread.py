




































from threading import Thread

from testrunner import TPSTestRunner

class TPSTestThread(Thread):

  def __init__(self, extensionDir, builddata=None, emailresults=False,
               testfile=None, logfile=None, rlock=None, config=None,
               autolog=False):
    assert(builddata)
    assert(config)
    self.extensionDir = extensionDir
    self.builddata = builddata
    self.emailresults = emailresults
    self.testfile = testfile
    self.logfile = logfile
    self.rlock = rlock
    self.config = config
    self.autolog = autolog
    Thread.__init__(self)

  def run(self):
    
    TPS = TPSTestRunner(self.extensionDir,
                        emailresults=self.emailresults,
                        testfile=self.testfile,
                        logfile=self.logfile,
                        binary=self.builddata['buildurl'],
                        config=self.config,
                        rlock=self.rlock,
                        mobile=False,
                        autolog=self.autolog)
    TPS.run_tests()

    
    
    binary = TPS.firefoxRunner.binary

    
    TPS_mobile = TPSTestRunner(self.extensionDir,
                               emailresults=self.emailresults,
                               testfile=self.testfile,
                               logfile=self.logfile,
                               binary=binary,
                               config=self.config,
                               rlock=self.rlock,
                               mobile=True,
                               autolog=self.autolog)
    TPS_mobile.run_tests()

    
    stageaccount = self.config.get('stageaccount')
    if stageaccount:
      username = stageaccount.get('username')
      password = stageaccount.get('password')
      passphrase = stageaccount.get('passphrase')
      if username and password and passphrase:
        stageconfig = self.config.copy()
        stageconfig['account'] = stageaccount.copy()
        TPS_stage = TPSTestRunner(self.extensionDir,
                                  emailresults=self.emailresults,
                                  testfile=self.testfile,
                                  logfile=self.logfile,
                                  binary=binary,
                                  config=stageconfig,
                                  rlock=self.rlock,
                                  mobile=False,
                                  autolog=self.autolog)
        TPS_stage.run_tests()
