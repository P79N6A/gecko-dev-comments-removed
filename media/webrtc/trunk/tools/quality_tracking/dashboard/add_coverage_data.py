
#-*- coding: utf-8 -*-








"""Implements a handler for adding coverage data."""

__author__ = 'phoglund@webrtc.org (Patrik HÃ¶glund)'

from datetime import datetime
import logging

from google.appengine.ext import db

import oauth_post_request_handler

REPORT_CATEGORIES = ('small_medium_tests', 'large_tests')


class CoverageData(db.Model):
  """This represents one coverage report from the build bot."""

  
  date = db.DateTimeProperty(required=True)

  
  line_coverage = db.FloatProperty(required=True)
  function_coverage = db.FloatProperty(required=True)
  branch_coverage = db.FloatProperty()

  
  report_category = db.CategoryProperty()


class AddCoverageData(oauth_post_request_handler.OAuthPostRequestHandler):
  """Used to report coverage data.

     Coverage data is reported as a POST request and should contain, aside from
     the regular oauth_* parameters, these values:

     date: The POSIX timestamp for when the coverage observation was made.
     report_category: A value in REPORT_CATEGORIES which characterizes the
         coverage information (e.g. is the coverage from small / medium tests
         or large tests?)

     line_coverage: Line coverage percentage.
     function_coverage: Function coverage percentage.
     branch_coverage: Branch coverage percentage.
  """

  def _parse_and_store_data(self):
    try:
      request_posix_timestamp = float(self.request.get('oauth_timestamp'))
      parsed_date = datetime.fromtimestamp(request_posix_timestamp)

      line_coverage = self._parse_percentage('line_coverage')
      function_coverage = self._parse_percentage('function_coverage')
      branch_coverage = self._parse_percentage('branch_coverage')
      report_category = self._parse_category('report_category')

    except ValueError as error:
      logging.warn('Invalid parameter in request: %s.' % error)
      self.response.set_status(400)
      return

    item = CoverageData(date=parsed_date,
                        line_coverage=line_coverage,
                        function_coverage=function_coverage,
                        branch_coverage=branch_coverage,
                        report_category=report_category)
    item.put()

  def _parse_percentage(self, key):
    """Parses out a percentage value from the request."""
    percentage = float(self.request.get(key))
    if percentage < 0.0 or percentage > 100.0:
      raise ValueError('%s is not a valid percentage.' % string_value)
    return percentage

  def _parse_category(self, key):
    value = self.request.get(key)
    if value in REPORT_CATEGORIES:
      return value
    else:
      raise ValueError("Invalid category %s." % value)
