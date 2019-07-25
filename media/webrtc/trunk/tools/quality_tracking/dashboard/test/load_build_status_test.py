
#-*- coding: utf-8 -*-








__author__ = 'phoglund@webrtc.org (Patrik HÃ¶glund)'

import unittest
from google.appengine.ext import db
from google.appengine.ext import testbed

from add_build_status_data import BuildStatusData
import load_build_status

class LoadBuildStatusTest(unittest.TestCase):
  def setUp(self):
     
    self.testbed = testbed.Testbed()
    
    self.testbed.activate()
    
    self.testbed.init_datastore_v3_stub()

  def test_returns_latest_nonbuilding_entries_when_loading_build_status(self):
    BuildStatusData(bot_name="Bot1", revision=17,
                    build_number=499, status="OK").put()
    BuildStatusData(bot_name="Bot2", revision=17,
                    build_number=505, status="OK").put()
    BuildStatusData(bot_name="Bot3", revision=17,
                    build_number=344, status="failed").put()
    BuildStatusData(bot_name="Bot1", revision=18,
                    build_number=499, status="building").put()
    BuildStatusData(bot_name="Bot2", revision=18,
                    build_number=505, status="failed").put()
    BuildStatusData(bot_name="Bot3", revision=18,
                    build_number=344, status="OK").put()

    loader = load_build_status.BuildStatusLoader()
    result = loader.load_build_status_data()

    self.assertEqual(3, len(result))

    
    
    self.assertEqual("Bot1", result[0].bot_name)
    self.assertEqual(17, result[0].revision)
    self.assertEqual("OK", result[0].status)

    self.assertEqual("Bot3", result[1].bot_name)
    self.assertEqual(18, result[1].revision)
    self.assertEqual("OK", result[1].status)

    self.assertEqual("Bot2", result[2].bot_name)
    self.assertEqual(18, result[2].revision)
    self.assertEqual("failed", result[2].status)

  def test_returns_lkgr_for_single_green_revision(self):
    BuildStatusData(bot_name="Bot1", revision=17,
                    build_number=499, status="OK").put()
    BuildStatusData(bot_name="Bot2", revision=17,
                    build_number=505, status="OK").put()
    BuildStatusData(bot_name="Bot3", revision=17,
                    build_number=344, status="OK").put()

    loader = load_build_status.BuildStatusLoader()
    self.assertEqual(17, loader.compute_lkgr())

  def test_returns_correct_lkgr_with_most_recent_revision_failed(self):
    BuildStatusData(bot_name="Bot1", revision=17,
                    build_number=499, status="OK").put()
    BuildStatusData(bot_name="Bot2", revision=17,
                    build_number=505, status="OK").put()
    BuildStatusData(bot_name="Bot3", revision=17,
                    build_number=344, status="OK").put()
    BuildStatusData(bot_name="Bot1", revision=18,
                    build_number=499, status="OK").put()
    BuildStatusData(bot_name="Bot2", revision=18,
                    build_number=505, status="failed").put()
    BuildStatusData(bot_name="Bot3", revision=18,
                    build_number=344, status="OK").put()

    loader = load_build_status.BuildStatusLoader()
    self.assertEqual(17, loader.compute_lkgr())

  def test_returns_none_if_no_revisions(self):
    loader = load_build_status.BuildStatusLoader()
    self.assertEqual(None, loader.compute_lkgr())

  def test_returns_none_if_no_green_revisions(self):
    BuildStatusData(bot_name="Bot2", revision=18,
                    build_number=505, status="failed").put()

    loader = load_build_status.BuildStatusLoader()
    self.assertEqual(None, loader.compute_lkgr())

  def test_skips_partially_building_revisions(self):
    BuildStatusData(bot_name="Bot1", revision=18,
                    build_number=499, status="building").put()
    BuildStatusData(bot_name="Bot2", revision=18,
                    build_number=505, status="OK").put()
    BuildStatusData(bot_name="Bot1", revision=17,
                    build_number=344, status="OK").put()

    loader = load_build_status.BuildStatusLoader()
    self.assertEqual(17, loader.compute_lkgr())


if __name__ == '__main__':
  unittest.main()
