import os
import sys
import unittest

sys.path.insert(1, os.path.abspath(os.path.join(__file__, "../..")))
import base_test


class ForwardToNothingTest(base_test.WebDriverBaseTest):
    
    def test_forwardToNothing(self):
        self.driver.get(self.webserver.where_is('navigation/forwardStart.html'))
        body = self.driver.find_element_by_css("body").text
        self.driver.go_forward()
        currbody = self.driver.find_element_by_css("body").text
        self.assertEqual(body, currbody)


if __name__ == '__main__':
    unittest.main()
