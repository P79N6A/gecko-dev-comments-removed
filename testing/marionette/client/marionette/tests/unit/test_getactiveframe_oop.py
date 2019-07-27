



from marionette_test import MarionetteTestCase


class TestGetActiveFrameOOP(MarionetteTestCase):
    def setUp(self):
        super(TestGetActiveFrameOOP, self).setUp()
        self.oop_by_default = self.marionette.execute_script("""
            try {
              return SpecialPowers.getBoolPref('dom.ipc.browser_frames.oop_by_default');
            }
            catch(e) {}
            """)
        self.mozBrowserFramesEnabled = self.marionette.execute_script("""
            try {
              return SpecialPowers.getBoolPref('dom.mozBrowserFramesEnabled');
            }
            catch(e) {}
            """)
        self.marionette.execute_script("""
            SpecialPowers.setBoolPref('dom.ipc.browser_frames.oop_by_default', true);
            """)
        self.marionette.execute_script("""
            SpecialPowers.setBoolPref('dom.mozBrowserFramesEnabled', true);
            """)

    def test_active_frame_oop(self):
        self.marionette.navigate(self.marionette.absolute_url("test.html"))
        self.marionette.execute_script("SpecialPowers.addPermission('browser', true, document)")

        
        self.marionette.execute_script("""
            let iframe1 = document.createElement("iframe");
            SpecialPowers.wrap(iframe1).mozbrowser = true;
            SpecialPowers.wrap(iframe1).remote = true;
            iframe1.id = "remote_iframe1";
            iframe1.style.height = "100px";
            iframe1.style.width = "100%%";
            iframe1.src = "%s";
            document.body.appendChild(iframe1);
            """ % self.marionette.absolute_url("test_oop_1.html"))

        
        self.assertEqual(self.marionette.get_active_frame(), None)
        self.assertTrue("test.html" in self.marionette.get_url())

        
        self.marionette.switch_to_frame('remote_iframe1')
        active_frame1 = self.marionette.get_active_frame()
        self.assertNotEqual(active_frame1.id, None)

        
        self.marionette.switch_to_frame()
        self.marionette.switch_to_frame(active_frame1)
        self.assertTrue("test_oop_1.html" in self.marionette.execute_script("return document.wrappedJSObject.location.href"))

        
        self.marionette.switch_to_frame()
        self.marionette.execute_script("""
            let iframe2 = document.createElement("iframe");
            SpecialPowers.wrap(iframe2).mozbrowser = true;
            SpecialPowers.wrap(iframe2).remote = true;
            iframe2.id = "remote_iframe2";
            iframe2.style.height = "100px";
            iframe2.style.width = "100%%";
            iframe2.src = "%s";
            document.body.appendChild(iframe2);
            """ % self.marionette.absolute_url("test_oop_2.html"))

        
        self.marionette.switch_to_frame('remote_iframe2')
        active_frame2 = self.marionette.get_active_frame()
        self.assertNotEqual(active_frame2.id, None)

        
        self.marionette.switch_to_frame()
        self.marionette.switch_to_frame(active_frame1)
        self.assertTrue("test_oop_1.html" in self.marionette.execute_script("return document.wrappedJSObject.location.href"))

        
        self.marionette.switch_to_frame()
        self.marionette.switch_to_frame(active_frame2)
        self.assertTrue("test_oop_2.html" in self.marionette.execute_script("return document.wrappedJSObject.location.href"))

        
        
        
        
        

    def tearDown(self):
        if self.oop_by_default is None:
            self.marionette.execute_script("""
                SpecialPowers.clearUserPref('dom.ipc.browser_frames.oop_by_default');
                """)
        else:
            self.marionette.execute_script("""
                SpecialPowers.setBoolPref('dom.ipc.browser_frames.oop_by_default', %s);
                """ % 'true' if self.oop_by_default else 'false')
        if self.mozBrowserFramesEnabled is None:
            self.marionette.execute_script("""
                SpecialPowers.clearUserPref('dom.mozBrowserFramesEnabled');
                """)
        else:
            self.marionette.execute_script("""
                SpecialPowers.setBoolPref('dom.mozBrowserFramesEnabled', %s);
                """ % 'true' if self.mozBrowserFramesEnabled else 'false')
