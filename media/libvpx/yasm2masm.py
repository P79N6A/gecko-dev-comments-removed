





































import sys, os.path, re

known_extern_types = {
  'rand': 'PROC',
  'vp8_bilinear_filters_mmx': 'DWORD'
}

comment_re = re.compile(r'(;.*)$')
global_re = re.compile(r'^\s*global ([\w()]+)')
extern_re = re.compile(r'^extern (sym\((\w+)\))')
movd_from_mem_re = re.compile(r'\b(movd\s+x?mm\d,\s*)(\[[^]]+\]|arg\(\d+\))')
movd_to_mem_re = re.compile(r'\b(movd\s*)(\[[^]]+\],\s*x?mm\d)')
label_re = re.compile(r'^\s*([\w()]+):')
times_hex_re = re.compile(r'^\s*times\s+(\d+)\s+(db|dw|dd)\s+0x([0-9a-fA-F]+)')
times_dec_re = re.compile(r'^\s*times\s+(\d+)\s+(db|dw|dd)\s+(-?\d+)')
mem_define_re = re.compile(r'^\s*%define\s*(\w+)\s*(\[[^]]+\])')
num_define_re = re.compile(r'^\s*%define\s*(\w+)\s*(\d+)')
preproc_re = re.compile(r'^\s*%(if|else|endif|elif)\b(.*)$')
ifidn_re = re.compile(r'^\s*%ifidn\b(.*)$')
undef_re = re.compile(r'%undef\s')
size_arg_re = re.compile(r'\b(byte|word|dword)\s+(arg\(\d+\)|\[[^]]+\])')
hex_re = re.compile(r'\b0x([0-9A-Fa-f]+)')

and_expr_re = re.compile(r'^(.*)&&(.*)$')
equal_expr_re = re.compile(r'^(.*)=(.*)$')
section_text_re = re.compile(r'^\s*section .text')

in_data = False
data_label = ''

def translate_expr(s):
    if and_expr_re.search(s):
        match = and_expr_re.search(s)
        return '(' + translate_expr(match.group(1)) + ' and ' + translate_expr(match.group(2)) + ')'
    if equal_expr_re.search(s):
        match = equal_expr_re.search(s)
        return '(' + translate_expr(match.group(1)) + ' eq ' + translate_expr(match.group(2)) + ')'
    return s

def type_of_extern(id):
    if id in known_extern_types:
        return known_extern_types[id]
    return 'ABS'

def translate(s):
    
    if s == '%include "vpx_ports/x86_abi_support.asm"':
        return 'include vpx_ports/x86_abi_support_win32.asm'

    if s == '%include "x86_abi_support.asm"':
        return 'include x86_abi_support_win32.asm'        
        
    
    if global_re.search(s):
        return global_re.sub('public \\1', s)

    
    if extern_re.search(s):
        match = extern_re.search(s)
        return extern_re.sub('extern \\1:' + type_of_extern(match.group(2)), s)

    
    if mem_define_re.search(s):
        return mem_define_re.sub('\\1 textequ <\\2>', s)
    if num_define_re.search(s):
        return num_define_re.sub('\\1 equ \\2', s)

    
    if preproc_re.search(s):
        match = preproc_re.search(s)
        if match.group(1) == 'elif':
            return 'elseif ' + translate_expr(match.group(2))
        return match.group(1) + translate_expr(match.group(2))

    if ifidn_re.search(s):
        return 'if 0'

    if section_text_re.search(s):
        return '.code';
        
    
    if undef_re.search(s):
        return ''

    
    if movd_from_mem_re.search(s):
        return movd_from_mem_re.sub('\\1dword ptr \\2', s)
    if movd_to_mem_re.search(s):
        return movd_to_mem_re.sub('\\1dword ptr \\2', s)

    
    if size_arg_re.search(s):
        return size_arg_re.sub('\\1 ptr \\2', s)

    
    
    global data_label
    if in_data and label_re.search(s):
        data_label = label_re.search(s).group(1)
        return ''
    if in_data and times_hex_re.search(s):
        match = times_hex_re.search(s)
        if (match.group(1) == '8' and match.group(2) == 'db') or \
           (match.group(1) == '4' and match.group(2) == 'dw'):
            
            print data_label + ' equ this qword'
            data_label = ''
        if match.group(1) == '16' and match.group(2) == 'db':
            
            print data_label + ' equ this xmmword'
            data_label = ''
        s = times_hex_re.sub(data_label + ' \\2 \\1 dup (0\\3h)', s)
        data_label = ''
        return s
    if in_data and times_dec_re.search(s):
        s = times_dec_re.sub(data_label + ' \\2 \\1 dup (\\3)', s)
        data_label = ''
        return s

    
    if hex_re.search(s):
        return hex_re.sub('0\\1h', s)

    return s

while 1:
    try:
        s = raw_input()
    except EOFError:
        break
    if s == 'SECTION_RODATA':
        in_data = True
    print translate(s)

print "end"
