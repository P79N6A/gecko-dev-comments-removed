



import os
import time
from marionette_test import MarionetteTestCase

class testPressRelease(MarionetteTestCase):
    def test_coordinates(self):
        testTouch = self.marionette.absolute_url("testAction.html")
        self.marionette.navigate(testTouch)
        button1 = self.marionette.find_element("id", "mozLink")
        new_id = button1.press(0, 400)
        button1.release(new_id, 0, 400)
        time.sleep(10)
        self.assertEqual("End", self.marionette.execute_script("return document.getElementById('mozLinkCopy').innerHTML;"))

    def test_no_coordinates(self):
      testTouch = self.marionette.absolute_url("testAction.html")
      self.marionette.navigate(testTouch)
      ele = self.marionette.find_element("id", "mozLinkCopy")
      new_id = ele.press()
      ele.release(new_id)
      time.sleep(10)
      self.assertEqual("End", self.marionette.execute_script("return document.getElementById('mozLinkCopy').innerHTML;"))

    def test_move(self):
      testTouch = self.marionette.absolute_url("testAction.html")
      self.marionette.navigate(testTouch)
      ele = self.marionette.find_element("id", "mozLink")
      press_id = ele.press()
      ele.release(press_id, 0, 300)
      time.sleep(10)
      self.assertEqual("Move", self.marionette.execute_script("return document.getElementById('mozLink').innerHTML;"))
      self.assertEqual("End", self.marionette.execute_script("return document.getElementById('mozLinkPos').innerHTML;"))
