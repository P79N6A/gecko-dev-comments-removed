# -*- coding: utf-8 -*-




from by import By
from marionette import Actions
from marionette_test import MarionetteTestCase
from selection import SelectionManager
from gestures import long_press_without_contextmenu


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

    def _first_word_location(self, el):
        '''Get the location (x, y) of the first word in el.

        Note: this function has a side effect which changes focus to the
        target element el.

        '''
        sel = SelectionManager(el)

        
        
        el.tap()
        sel.move_caret_to_front()
        sel.move_caret_by_offset(1)

        return sel.caret_location()

    def _long_press_to_select(self, el, x, y):
        '''Long press the location (x, y) to select a word.

        SelectionCarets should appear. On Windows, those spaces after the
        word will also be selected.

        '''
        long_press_without_contextmenu(self.marionette, el, self._long_press_time,
                                       x, y)

    def _test_long_press_to_select_a_word(self, el, assertFunc):
        sel = SelectionManager(el)
        original_content = sel.content
        words = original_content.split()
        self.assertTrue(len(words) >= 2, 'Expect at least two words in the content.')
        target_content = words[0]

        
        x, y = self._first_word_location(el)
        self._long_press_to_select(el, x, y)

        
        assertFunc(target_content, sel.selected_content.rstrip())

    def _test_move_selection_carets(self, el, assertFunc):
        sel = SelectionManager(el)
        original_content = sel.content
        words = original_content.split()
        self.assertTrue(len(words) >= 1, 'Expect at least one word in the content.')

        
        target_content = original_content[len(words[0]):]

        
        
        el.tap()
        sel.select_all()
        (_, _), (end_caret_x, end_caret_y) = sel.selection_carets_location()

        x, y = self._first_word_location(el)
        self._long_press_to_select(el, x, y)

        
        (caret1_x, caret1_y), (caret2_x, caret2_y) = sel.selection_carets_location()
        self.actions.flick(el, caret2_x, caret2_y, end_caret_x, end_caret_y).perform()

        
        self.actions.flick(el, caret1_x, caret2_y, caret2_x, caret2_y).perform()

        
        assertFunc(target_content.lstrip(), sel.selected_content.lstrip())

    def _test_minimum_select_one_character(self, el, assertFunc,
                                           x=None, y=None):
        sel = SelectionManager(el)
        original_content = sel.content
        words = original_content.split()
        self.assertTrue(len(words) >= 1, 'Expect at least one word in the content.')

        
        target_content = original_content[0]

        if x and y:
            
            
            pass
        else:
            x, y = self._first_word_location(el)
        self._long_press_to_select(el, x, y)

        
        (caret1_x, caret1_y), (caret2_x, caret2_y) = sel.selection_carets_location()
        self.actions.flick(el, caret2_x, caret2_y, caret1_x, caret1_y,).perform()

        assertFunc(target_content, sel.selected_content)

    def _test_focus_obtained_by_long_press(self, el1, el2):
        '''Test the focus could be changed from el1 to el2 by long press.

        If the focus is changed to e2 successfully, SelectionCarets should
        appear and could be dragged.

        '''
        
        

        
        
        
        x, y = self._first_word_location(el2)
        el1.tap()
        self._test_minimum_select_one_character(el2, self.assertEqual,
                                                x=x, y=y)

    
    
    
    def test_input_long_press_to_select_a_word(self):
        self.openTestHtml(enabled=True)
        self._test_long_press_to_select_a_word(self._input, self.assertEqual)

    def test_input_move_selection_carets(self):
        self.openTestHtml(enabled=True)
        self._test_move_selection_carets(self._input, self.assertEqual)

    def test_input_minimum_select_one_caracter(self):
        self.openTestHtml(enabled=True)
        self._test_minimum_select_one_character(self._input, self.assertEqual)

    def test_input_focus_obtained_by_long_press_from_textarea(self):
        self.openTestHtml(enabled=True)
        self._test_focus_obtained_by_long_press(self._textarea, self._input)

    def test_input_focus_obtained_by_long_press_from_contenteditable(self):
        self.openTestHtml(enabled=True)
        self._test_focus_obtained_by_long_press(self._contenteditable, self._input)

    def test_input_focus_obtained_by_long_press_from_content_non_editable(self):
        self.openTestHtml(enabled=True)
        self._test_focus_obtained_by_long_press(self._content, self._input)

    
    
    
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

    def test_textarea_focus_obtained_by_long_press_from_input(self):
        self.openTestHtml(enabled=True)
        self._test_focus_obtained_by_long_press(self._input, self._textarea)

    def test_textarea_focus_obtained_by_long_press_from_contenteditable(self):
        self.openTestHtml(enabled=True)
        self._test_focus_obtained_by_long_press(self._contenteditable, self._textarea)

    def test_textarea_focus_obtained_by_long_press_from_content_non_editable(self):
        self.openTestHtml(enabled=True)
        self._test_focus_obtained_by_long_press(self._content, self._textarea)

    
    
    
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

    def test_contenteditable_focus_obtained_by_long_press_from_input(self):
        self.openTestHtml(enabled=True)
        self._test_focus_obtained_by_long_press(self._input, self._contenteditable)

    def test_contenteditable_focus_obtained_by_long_press_from_textarea(self):
        self.openTestHtml(enabled=True)
        self._test_focus_obtained_by_long_press(self._textarea, self._contenteditable)

    def test_contenteditable_focus_obtained_by_long_press_from_content_non_editable(self):
        self.openTestHtml(enabled=True)
        self._test_focus_obtained_by_long_press(self._content, self._contenteditable)

    
    
    
    def test_contenteditable_long_press_to_select_a_word_disabled(self):
        self.openTestHtml(enabled=False)
        self._test_long_press_to_select_a_word(self._contenteditable, self.assertNotEqual)

    def test_contenteditable_move_selection_carets_disabled(self):
        self.openTestHtml(enabled=False)
        self._test_move_selection_carets(self._contenteditable, self.assertNotEqual)

    
    
    
    def test_content_non_editable_minimum_select_one_character_by_selection(self):
        self.openTestHtml(enabled=True)
        self._test_minimum_select_one_character(self._content, self.assertEqual)

    def test_content_non_editable_focus_obtained_by_long_press_from_input(self):
        self.openTestHtml(enabled=True)
        self._test_focus_obtained_by_long_press(self._input, self._content)

    def test_content_non_editable_focus_obtained_by_long_press_from_textarea(self):
        self.openTestHtml(enabled=True)
        self._test_focus_obtained_by_long_press(self._textarea, self._content)

    def test_content_non_editable_focus_obtained_by_long_press_from_contenteditable(self):
        self.openTestHtml(enabled=True)
        self._test_focus_obtained_by_long_press(self._contenteditable, self._content)

