





import buildconfig

from mozbuild.preprocessor import preprocess









def _do_preprocessing(output_svg, input_svg_file, additional_defines):
    additional_defines.update(buildconfig.defines)
    preprocess(output=output_svg,
               includes=[input_svg_file],
               marker='%',
               defines=additional_defines)

def tab_side_start(output_svg, input_svg_file):
    _do_preprocessing(output_svg, input_svg_file, {'TAB_SIDE': 'start'})

def tab_side_end(output_svg, input_svg_file):
    _do_preprocessing(output_svg, input_svg_file, {'TAB_SIDE': 'end'})

def aero_tab_side_start(output_svg, input_svg_file):
    _do_preprocessing(output_svg, input_svg_file,
                      {'TAB_SIDE': 'start',
                       'WINDOWS_AERO': 1})

def aero_tab_side_end(output_svg, input_svg_file):
    _do_preprocessing(output_svg, input_svg_file,
                      {'TAB_SIDE': 'end',
                       'WINDOWS_AERO': 1})

