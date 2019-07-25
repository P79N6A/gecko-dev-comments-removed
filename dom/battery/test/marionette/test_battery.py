import unittest
from marionette_test import MarionetteTestCase


class BatteryTest(MarionetteTestCase):

    @unittest.expectedFailure
    def test_chargingchange(self):
        marionette = self.marionette
        self.assertTrue(marionette.emulator.is_running)
        marionette.set_script_timeout(10000)

        moz_charging = marionette.execute_script("return navigator.mozBattery.charging;")
        emulator_charging = marionette.emulator.battery.charging
        self.assertEquals(moz_charging, emulator_charging)

        
        
        self.assertTrue(marionette.execute_script("""
        window.wrappedJSObject._chargingchanged = false;
        navigator.mozBattery.addEventListener("chargingchange", function() {
            window.wrappedJSObject._chargingchanged = true;
        });
        return true;
    """))

        
        marionette.emulator.battery.charging = not emulator_charging
        new_emulator_charging_state = marionette.emulator.battery.charging
        self.assertEquals(new_emulator_charging_state, (not emulator_charging))

        
        charging_changed = marionette.execute_async_script("""
        var callback = arguments[arguments.length - 1];
        function check_charging_change() {
            if (window.wrappedJSObject._chargingchanged) {
                callback(window.wrappedJSObject._chargingchanged);
            }
            else {
                setTimeout(check_charging_change, 500);
            }
        }
        setTimeout(check_charging_change, 0);
    """)
        self.assertTrue(charging_changed)

        
        
        if not new_emulator_charging_state:
            marionette.emulator.battery.charging = True

    def test_levelchange(self):
        marionette = self.marionette
        self.assertTrue(marionette.emulator.is_running)
        marionette.set_script_timeout(10000)

        
        
        moz_level = marionette.execute_script("return navigator.mozBattery.level;")
        self.assertEquals(moz_level, marionette.emulator.battery.level)

        
        
        self.assertTrue(marionette.execute_script("""
        window.wrappedJSObject._levelchanged = false;
        navigator.mozBattery.addEventListener("levelchange", function() {
            window.wrappedJSObject._levelchanged = true;
        });
        return true;
    """))

        
        if moz_level > 0.2:
            new_level = moz_level - 0.1
        else:
            new_level = moz_level + 0.1
        marionette.emulator.battery.level = new_level

        
        moz_level = marionette.emulator.battery.level
        self.assertEquals(int(new_level * 100), int(moz_level * 100))

        
        level_changed = marionette.execute_async_script("""
        var callback = arguments[arguments.length - 1];
        function check_level_change() {
            if (window.wrappedJSObject._levelchanged) {
                callback(window.wrappedJSObject._levelchanged);
            }
            else {
                setTimeout(check_level_change, 500);
            }
        }
        setTimeout(check_level_change, 0);
    """)
        self.assertTrue(level_changed)



