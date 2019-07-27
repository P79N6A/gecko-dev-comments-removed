import os
import sys
import unittest
import ConfigParser

sys.path.insert(1, os.path.abspath(os.path.join(__file__, "../..")))
import base_test
from webserver import Httpd


class WebDriverAuthTest(unittest.TestCase):

    
    
    @classmethod
    def setUpClass(cls):
        cls.driver = base_test.create_driver()

        def basic_response_func( request, *args ):
            return (401, {"WWW-Authenticate" : "Basic"}, None)

        basic_auth_handler = { 'method': 'GET',
                               'path' : '/navigation/auth_required_basic',
                               'function' : basic_response_func }
        urlhandlers = [ basic_auth_handler ]

        cls.webserver = Httpd( urlhandlers=urlhandlers )
        cls.webserver.start()

    @classmethod
    def tearDownClass(cls):
        cls.driver.quit()
        cls.webserver.stop()

    
    def test_response_401_auth_basic(self):
        page = self.webserver.where_is('navigation/auth_required_basic')
        self.driver.set_page_load_timeout(5)
        try:
            self.driver.get( page )
            
            self.assertTrue(True)
        except exceptions.TimeoutException:
            self.fail("Did not get response from browser.")
        except:
            self.fail("Unexpected failure. Please investigate.")

if __name__ == "__main__":
    unittest.main()
