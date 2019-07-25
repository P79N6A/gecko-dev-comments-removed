
#-*- coding: utf-8 -*-








"""Loads coverage data from the database."""

__author__ = 'phoglund@webrtc.org (Patrik HÃ¶glund)'

import logging

from google.appengine.ext import db
import gviz_api


class CoverageDataLoader:
  """ Loads coverage data from the database."""

  def load_coverage_json_data(self, report_category):
    coverage_entries = db.GqlQuery('SELECT * '
                                   'FROM CoverageData '
                                   'WHERE report_category = :1 '
                                   'ORDER BY date ASC', report_category)
    data = []
    for coverage_entry in coverage_entries:
      
      
      
      data.append({'aa_date': coverage_entry.date,
                   'line_coverage': coverage_entry.line_coverage,
                   'function_coverage': coverage_entry.function_coverage,
                   'branch_coverage': coverage_entry.branch_coverage,
                  })

    description = {
        'aa_date': ('datetime', 'Date'),
        'line_coverage': ('number', 'Line Coverage'),
        'function_coverage': ('number', 'Function Coverage'),
        'branch_coverage': ('number', 'Branch Coverage'),
    }
    coverage_data = gviz_api.DataTable(description, data)
    return coverage_data.ToJSon(order_by='date')
