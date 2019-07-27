import os
import sys
import unittest

sys.path.insert(1, os.path.abspath(os.path.join(__file__, "../..")))
import base_test


class RefreshPageTest(base_test.WebDriverBaseTest):
    
    def test_refreshPage(self):
        self.driver.get(self.webserver.where_is('navigation/res/refreshPageStatic.html'))
        body = self.driver.find_element_by_css("body").get_text()
        self.driver.execute_script("document.getElementById('body').innerHTML=''")
        self.driver.refresh()
        newbody = self.driver.find_element_by_css("body").get_text()
        self.assertEqual(body, newbody)

        self.driver.get(self.webserver.where_is('navigation/res/refreshPageDynamic.html'))
        body = self.driver.find_element_by_css("body").get_text()
        self.driver.refresh()
        newbody = self.driver.find_element_by_css("body").get_text()
        self.assertNotEqual(body, newbody)


if __name__ == '__main__':
    unittest.main()
