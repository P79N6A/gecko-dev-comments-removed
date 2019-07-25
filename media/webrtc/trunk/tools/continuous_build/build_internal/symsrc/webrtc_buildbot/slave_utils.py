








__author__ = 'kjellander@webrtc.org (Henrik Kjellander)'

"""Contains a customized GClient class for WebRTC use.

It is essential that this class is imported after the chromium_commands so it
can overwrite its registration of the 'gclient' command.
This should by done by adding:

from webrtc_buildbot import slave_utils

into the buildbot.tac file of the try slave.
"""

from slave import chromium_commands

try:
  
  
  
  from buildslave.commands.registry import commandRegistry
except ImportError:
  
  
  
  from buildbot.slave.registry import registerSlaveCommand

class GClient(chromium_commands.GClient):
  def doPatch(self, res):
    
    
    
    
    
    self.patch = (self.patch[0], self.patch[1], 'trunk')
    return chromium_commands.GClient.doPatch(self, res)


def RegisterGclient():
  try:
    

    
    
    commandRegistry['gclient'] = 'webrtc_buildbot.slave_utils.GClient'
    return
  except (AssertionError, NameError):
    pass

  try:
    
    
    
    registerSlaveCommand('gclient', GClient, commands.command_version)
  except (AssertionError, NameError):
    pass

RegisterGclient()
