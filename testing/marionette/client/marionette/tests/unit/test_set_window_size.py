



from marionette_driver.errors import MarionetteException
from marionette import MarionetteTestCase

class TestSetWindowSize(MarionetteTestCase):
    def setUp(self):
        super(MarionetteTestCase, self).setUp()
        self.start_size = self.marionette.window_size
        self.max_width = self.marionette.execute_script("return window.screen.availWidth;")
        self.max_height = self.marionette.execute_script("return window.screen.availHeight;")

    def tearDown(self):
        
        
        
        
        if self.start_size['width'] == self.max_width and self.start_size['height'] == self.max_height:
            self.start_size['width']-=1
        self.marionette.set_window_size(self.start_size['width'], self.start_size['height'])
        super(MarionetteTestCase, self).tearDown()

    def test_that_we_can_get_and_set_window_size(self):
        
        self.marionette.execute_script("""
        window.wrappedJSObject.rcvd_event = false;
        window.onresize = function() {
            window.wrappedJSObject.rcvd_event = true;
        };
        """)

        
        width = self.max_width - 100
        height = self.max_height - 100
        self.marionette.set_window_size(width, height)
        self.wait_for_condition(lambda m: m.execute_script("return window.wrappedJSObject.rcvd_event;"))
        size = self.marionette.window_size
        self.assertEqual(size['width'], width,
                         "Window width is %s but should be %s" % (size['width'], width))
        self.assertEqual(size['height'], height,
                         "Window height is %s but should be %s" % (size['height'], height))

    def test_that_we_throw_an_error_when_trying_to_set_maximum_size(self):
        
        width = self.max_width - 100
        height = self.max_height - 100
        self.marionette.set_window_size(width, height)
        
        with self.assertRaisesRegexp(MarionetteException, "Invalid requested size"):
            self.marionette.set_window_size(self.max_width, self.max_height)
        size = self.marionette.window_size
        self.assertEqual(size['width'], width, "Window width should not have changed")
        self.assertEqual(size['height'], height, "Window height should not have changed")

    def test_that_we_can_maximise_the_window(self):
        
        width = self.max_width - 100
        height = self.max_height - 100
        self.marionette.set_window_size(width, height)

        
        self.marionette.execute_script("""
        window.wrappedJSObject.rcvd_event = false;
        window.onresize = function() {
            window.wrappedJSObject.rcvd_event = true;
        };
        """)
        self.marionette.maximize_window()
        self.wait_for_condition(lambda m: m.execute_script("return window.wrappedJSObject.rcvd_event;"))

        size = self.marionette.window_size
        self.assertEqual(size['width'], self.max_width, "Window width does not use availWidth")
        self.assertEqual(size['height'], self.max_height, "Window height does not use availHeight")
