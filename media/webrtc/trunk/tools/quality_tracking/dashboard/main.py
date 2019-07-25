
#-*- coding: utf-8 -*-








"""Connects all URLs with their respective handlers."""

__author__ = 'phoglund@webrtc.org (Patrik HÃ¶glund)'

from google.appengine.ext.webapp import template
import webapp2

import add_build_status_data
import add_coverage_data
import dashboard
import lkgr_page

app = webapp2.WSGIApplication([('/', dashboard.ShowDashboard),
                               ('/lkgr', lkgr_page.ShowLkgr),
                               ('/add_coverage_data',
                                add_coverage_data.AddCoverageData),
                               ('/add_build_status_data',
                                add_build_status_data.AddBuildStatusData)],
                              debug=True)