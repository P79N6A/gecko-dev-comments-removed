
#-*- coding: utf-8 -*-








"""Implements the LKGR page."""

__author__ = 'phoglund@webrtc.org (Patrik HÃ¶glund)'

import webapp2

import load_build_status

class ShowLkgr(webapp2.RequestHandler):
  """This handler shows the LKGR in the simplest possible way.

     The page is intended to be used by automated tools.
  """
  def get(self):
    build_status_loader = load_build_status.BuildStatusLoader()

    lkgr = build_status_loader.compute_lkgr()
    if lkgr is None:
      self.response.out.write('No data has been uploaded to the dashboard.')
    else:
      self.response.out.write(lkgr)
