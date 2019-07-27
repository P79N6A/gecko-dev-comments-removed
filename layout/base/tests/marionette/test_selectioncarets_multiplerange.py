# -*- coding: utf-8 -*-




from marionette_driver.by import By
from marionette_driver.marionette import Actions
from marionette import MarionetteTestCase
from marionette_driver.selection import SelectionManager
from marionette_driver.gestures import long_press_without_contextmenu


class SelectionCaretsMultipleRangeTest(MarionetteTestCase):
    _long_press_time = 1        

    def setUp(self):
        
        MarionetteTestCase.setUp(self)
        self.actions = Actions(self.marionette)

    def openTestHtml(self, enabled=True):
        
        
        self.marionette.execute_async_script(
            'SpecialPowers.pushPrefEnv({"set": [["selectioncaret.enabled", %s],["selectioncaret.noneditable", %s]]}, marionetteScriptFinished);' %
            ( ('true' if enabled else 'false'),  ('true' if enabled else 'false')))

        test_html = self.marionette.absolute_url('test_selectioncarets_multiplerange.html')
        self.marionette.navigate(test_html)

        self._body = self.marionette.find_element(By.ID, 'bd')
        self._sel1 = self.marionette.find_element(By.ID, 'sel1')
        self._sel2 = self.marionette.find_element(By.ID, 'sel2')
        self._sel3 = self.marionette.find_element(By.ID, 'sel3')
        self._sel4 = self.marionette.find_element(By.ID, 'sel4')
        self._sel6 = self.marionette.find_element(By.ID, 'sel6')
        self._nonsel1 = self.marionette.find_element(By.ID, 'nonsel1')

    def openTestHtml2(self, enabled=True):
        
        self.marionette.execute_script(
            'SpecialPowers.setBoolPref("selectioncaret.enabled", %s);' %
            ('true' if enabled else 'false'))

        test_html2 = self.marionette.absolute_url('test_selectioncarets_longtext.html')
        self.marionette.navigate(test_html2)

        self._body2 = self.marionette.find_element(By.ID, 'bd2')
        self._longtext = self.marionette.find_element(By.ID, 'longtext')

    def openTestHtml3(self, enabled=True):
        
        self.marionette.execute_script(
            'SpecialPowers.setBoolPref("selectioncaret.enabled", %s);' %
            ('true' if enabled else 'false'))

        test_html3 = self.marionette.absolute_url('test_selectioncarets_iframe.html')
        self.marionette.navigate(test_html3)

        self._iframe = self.marionette.find_element(By.ID, 'frame')

    def _long_press_to_select_word(self, el, wordOrdinal):
        sel = SelectionManager(el)
        original_content = sel.content
        words = original_content.split()
        self.assertTrue(wordOrdinal < len(words),
            'Expect at least %d words in the content.' % wordOrdinal)

        
        offset = 0
        for i in range(wordOrdinal):
            offset += (len(words[i]) + 1)

        
        el.tap()
        sel.move_caret_to_front()
        sel.move_caret_by_offset(offset)
        x, y = sel.caret_location()

        
        
        
        long_press_without_contextmenu(self.marionette, el, self._long_press_time, x, y)

    def _to_unix_line_ending(self, s):
        """Changes all Windows/Mac line endings in s to UNIX line endings."""

        return s.replace('\r\n', '\n').replace('\r', '\n')

    def test_long_press_to_select_non_selectable_word(self):
        '''Testing long press on non selectable field.
        We should not select anything when long press on non selectable fields.'''

        self.openTestHtml(enabled=True)
        halfY = self._nonsel1.size['height'] / 2
        long_press_without_contextmenu(self.marionette, self._nonsel1, self._long_press_time, 0, halfY)
        sel = SelectionManager(self._nonsel1)
        range_count = sel.range_count()
        self.assertEqual(range_count, 0)

    def test_drag_caret_over_non_selectable_field(self):
        '''Testing drag caret over non selectable field.
        So that the selected content should exclude non selectable field and
        end selection caret should appear in last range's position.'''
        self.openTestHtml(enabled=True)

        
        self._long_press_to_select_word(self._sel4, 3)
        sel = SelectionManager(self._body)
        (_, _), (end_caret_x, end_caret_y) = sel.selection_carets_location()

        self._long_press_to_select_word(self._sel6, 0)
        (_, _), (end_caret2_x, end_caret2_y) = sel.selection_carets_location()

        
        self._long_press_to_select_word(self._sel3, 3)

        
        (caret1_x, caret1_y), (caret2_x, caret2_y) = sel.selection_carets_location()
        self.actions.flick(self._body, caret2_x, caret2_y, end_caret_x, end_caret_y, 1).perform()
        self.assertEqual(self._to_unix_line_ending(sel.selected_content.strip()),
            'this 3\nuser can select this')

        (caret1_x, caret1_y), (caret2_x, caret2_y) = sel.selection_carets_location()
        self.actions.flick(self._body, caret2_x, caret2_y, end_caret2_x, end_caret2_y, 1).perform()
        self.assertEqual(self._to_unix_line_ending(sel.selected_content.strip()),
            'this 3\nuser can select this 4\nuser can select this 5\nuser')

    def test_drag_caret_to_beginning_of_a_line(self):
        '''Bug 1094056
        Test caret visibility when caret is dragged to beginning of a line
        '''
        self.openTestHtml(enabled=True)

        
        self._long_press_to_select_word(self._sel2, 0)
        sel = SelectionManager(self._body)
        (start_caret_x, start_caret_y), (end_caret_x, end_caret_y) = sel.selection_carets_location()

        
        self._long_press_to_select_word(self._sel1, 2)

        
        (caret1_x, caret1_y), (caret2_x, caret2_y) = sel.selection_carets_location()
        self.actions.flick(self._body, caret2_x, caret2_y, start_caret_x, start_caret_y).perform()

        
        self.actions.flick(self._body, start_caret_x, start_caret_y, caret2_x, caret2_y).perform()

        self.assertEqual(self._to_unix_line_ending(sel.selected_content.strip()), 'select')

    def test_caret_position_after_changing_orientation_of_device(self):
        '''Bug 1094072
        If positions of carets are updated correctly, they should be draggable.
        '''
        
        if not self.marionette.session_capabilities['rotatable']:
            return

        self.openTestHtml2(enabled=True)

        
        self.marionette.set_orientation('portrait')
        self._long_press_to_select_word(self._longtext, 12)
        sel = SelectionManager(self._body2)
        (p_start_caret_x, p_start_caret_y), (p_end_caret_x, p_end_caret_y) = sel.selection_carets_location()
        self.marionette.set_orientation('landscape')
        (l_start_caret_x, l_start_caret_y), (l_end_caret_x, l_end_caret_y) = sel.selection_carets_location()

        
        self.actions.flick(self._body2, l_end_caret_x, l_end_caret_y, l_start_caret_x, l_start_caret_y).perform()

        
        
        self.marionette.set_orientation('portrait')

        self.assertEqual(self._to_unix_line_ending(sel.selected_content.strip()), 'o')

    def test_select_word_inside_an_iframe(self):
        '''Bug 1088552
        The scroll offset in iframe should be taken into consideration properly.
        In this test, we scroll content in the iframe to the bottom to cause a
        huge offset. If we use the right coordinate system, selection should
        work. Otherwise, it would be hard to trigger select word.
        '''
        self.openTestHtml3(enabled=True)

        
        self.marionette.switch_to_frame(self._iframe)
        self.marionette.execute_script(
         'document.getElementById("bd2").scrollTop += 999')

        
        self._body2 = self.marionette.find_element(By.ID, 'bd2')
        sel = SelectionManager(self._body2)
        self._bottomtext = self.marionette.find_element(By.ID, 'bottomtext')
        long_press_without_contextmenu(self.marionette, self._bottomtext, self._long_press_time)

        self.assertNotEqual(self._to_unix_line_ending(sel.selected_content.strip()), '')
