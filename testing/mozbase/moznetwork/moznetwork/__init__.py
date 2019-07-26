



"""
moznetwork is a very simple module designed for one task: getting the
network address of the current machine.

Example usage:

::

  import moznetwork

  try:
      ip = moznetwork.get_ip()
      print "The external IP of your machine is '%s'" % ip
  except moznetwork.NetworkError:
      print "Unable to determine IP address of machine"
      raise

"""

from moznetwork import *
