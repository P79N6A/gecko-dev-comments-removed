# -*- coding: utf-8 -*-




from by import By
from marionette import Actions
from marionette_test import MarionetteTestCase
from selection import SelectionManager


class SelectionCaretsTest(MarionetteTestCase):
    _long_press_time = 1        
    _input_selector = (By.ID, 'input')
    _textarea_selector = (By.ID, 'textarea')
    _textarea_rtl_selector = (By.ID, 'textarea_rtl')
    _contenteditable_selector = (By.ID, 'contenteditable')
    _content_selector = (By.ID, 'content')

    def setUp(self):
        
        MarionetteTestCase.setUp(self)
        self.actions = Actions(self.marionette)

    def openTestHtml(self, enabled=True):
        '''Open html for testing and locate elements, and enable/disable touch
        caret.'''
        self.marionette.execute_script(
            'SpecialPowers.setBoolPref("selectioncaret.enabled", %s);' %
            ('true' if enabled else 'false'))

        test_html = self.marionette.absolute_url('test_selectioncarets.html')
        self.marionette.navigate(test_html)

        self._input = self.marionette.find_element(*self._input_selector)
        self._textarea = self.marionette.find_element(*self._textarea_selector)
        self._textarea_rtl = self.marionette.find_element(*self._textarea_rtl_selector)
        self._contenteditable = self.marionette.find_element(*self._contenteditable_selector)
        self._content = self.marionette.find_element(*self._content_selector)

    def _long_press_to_select_first_word(self, el, sel):
        
        el.tap()
        sel.move_caret_to_front()
        sel.move_caret_by_offset(1)
        x, y = sel.caret_location()

        
        
        self.actions.long_press(el, self._long_press_time, x, y).perform()

    def _test_long_press_to_select_a_word(self, el, assertFunc):
        sel = SelectionManager(el)
        original_content = sel.content
        words = original_content.split()
        self.assertTrue(len(words) >= 2, 'Expect at least two words in the content.')

        
        content_to_add = '!'
        target_content = original_content.replace(words[0], content_to_add, 1)

        self._long_press_to_select_first_word(el, sel)

        
        el.send_keys(content_to_add)
        assertFunc(target_content, sel.content)

    def _test_move_selection_carets(self, el, assertFunc):
        sel = SelectionManager(el)
        original_content = sel.content
        words = original_content.split()
        self.assertTrue(len(words) >= 1, 'Expect at least one word in the content.')

        
        content_to_add = '!'
        target_content = words[0] + content_to_add

        
        
        el.tap()
        sel.select_all()
        (_, _), (end_caret_x, end_caret_y) = sel.selection_carets_location()

        self._long_press_to_select_first_word(el, sel)

        
        (caret1_x, caret1_y), (caret2_x, caret2_y) = sel.selection_carets_location()
        self.actions.flick(el, caret2_x, caret2_y, end_caret_x, end_caret_y).perform()

        
        self.actions.flick(el, caret1_x, caret2_y, caret2_x, caret2_y).perform()

        el.send_keys(content_to_add)
        assertFunc(target_content, sel.content)

    def _test_minimum_select_one_character(self, el, assertFunc):
        sel = SelectionManager(el)
        original_content = sel.content
        words = original_content.split()
        self.assertTrue(len(words) >= 1, 'Expect at least one word in the content.')

        
        content_to_add = '!'
        new_word = words[0][:-1] + content_to_add
        target_content = original_content.replace(words[0], new_word, 1)

        self._long_press_to_select_first_word(el, sel)

        
        (caret1_x, caret1_y), (caret2_x, caret2_y) = sel.selection_carets_location()
        self.actions.flick(el, caret1_x, caret1_y, caret2_x, caret2_y,).perform()

        
        el.send_keys(content_to_add)
        assertFunc(target_content, sel.content)

    def _test_minimum_select_one_character_by_selection(self, el, assertFunc):
        sel = SelectionManager(el)
        el.tap()
        sel.select_all()

        
        
        
        (caret1_x, caret1_y), (caret2_x, caret2_y) = sel.selection_carets_location()
        self.actions.flick(el, caret2_x, caret2_y, caret1_x, caret1_y).perform()

        cmd = sel.js_selection_cmd() +\
            '''return (sel.anchorNode == sel.focusNode) &&
                      (sel.focusOffset - sel.anchorOffset == 1);'''
        result = self.marionette.execute_script(cmd, script_args=[el])

        assertFunc(result)

    
    
    
    def test_input_long_press_to_select_a_word(self):
        self.openTestHtml(enabled=True)
        self._test_long_press_to_select_a_word(self._input, self.assertEqual)

    def test_input_move_selection_carets(self):
        self.openTestHtml(enabled=True)
        self._test_move_selection_carets(self._input, self.assertEqual)

    def test_input_minimum_select_one_caracter(self):
        self.openTestHtml(enabled=True)
        self._test_minimum_select_one_character(self._input, self.assertEqual)

    
    
    
    def test_input_long_press_to_select_a_word_disabled(self):
        self.openTestHtml(enabled=False)
        self._test_long_press_to_select_a_word(self._input, self.assertNotEqual)

    def test_input_move_selection_carets_disabled(self):
        self.openTestHtml(enabled=False)
        self._test_move_selection_carets(self._input, self.assertNotEqual)

    
    
    
    def test_textarea_long_press_to_select_a_word(self):
        self.openTestHtml(enabled=True)
        self._test_long_press_to_select_a_word(self._textarea, self.assertEqual)

    def test_textarea_move_selection_carets(self):
        self.openTestHtml(enabled=True)
        self._test_move_selection_carets(self._textarea, self.assertEqual)

    def test_textarea_minimum_select_one_caracter(self):
        self.openTestHtml(enabled=True)
        self._test_minimum_select_one_character(self._textarea, self.assertEqual)

    
    
    
    def test_textarea_long_press_to_select_a_word_disabled(self):
        self.openTestHtml(enabled=False)
        self._test_long_press_to_select_a_word(self._textarea, self.assertNotEqual)

    def test_textarea_move_selection_carets_disable(self):
        self.openTestHtml(enabled=False)
        self._test_move_selection_carets(self._textarea, self.assertNotEqual)

    
    
    
    def test_textarea_rtl_long_press_to_select_a_word(self):
        self.openTestHtml(enabled=True)
        self._test_long_press_to_select_a_word(self._textarea_rtl, self.assertEqual)

    def test_textarea_rtl_move_selection_carets(self):
        self.openTestHtml(enabled=True)
        self._test_move_selection_carets(self._textarea_rtl, self.assertEqual)

    def test_textarea_rtl_minimum_select_one_caracter(self):
        self.openTestHtml(enabled=True)
        self._test_minimum_select_one_character(self._textarea_rtl, self.assertEqual)

    
    
    
    def test_textarea_rtl_long_press_to_select_a_word_disabled(self):
        self.openTestHtml(enabled=False)
        self._test_long_press_to_select_a_word(self._textarea_rtl, self.assertNotEqual)

    def test_textarea_rtl_move_selection_carets_disabled(self):
        self.openTestHtml(enabled=False)
        self._test_move_selection_carets(self._textarea_rtl, self.assertNotEqual)

    
    
    
    def test_contenteditable_long_press_to_select_a_word(self):
        self.openTestHtml(enabled=True)
        self._test_long_press_to_select_a_word(self._contenteditable, self.assertEqual)

    def test_contenteditable_move_selection_carets(self):
        self.openTestHtml(enabled=True)
        self._test_move_selection_carets(self._contenteditable, self.assertEqual)

    def test_contenteditable_minimum_select_one_character(self):
        self.openTestHtml(enabled=True)
        self._test_minimum_select_one_character(self._contenteditable, self.assertEqual)

    
    
    
    def test_contenteditable_long_press_to_select_a_word_disabled(self):
        self.openTestHtml(enabled=False)
        self._test_long_press_to_select_a_word(self._contenteditable, self.assertNotEqual)

    def test_contenteditable_move_selection_carets_disabled(self):
        self.openTestHtml(enabled=False)
        self._test_move_selection_carets(self._contenteditable, self.assertNotEqual)

    
    
    
    
    
    
