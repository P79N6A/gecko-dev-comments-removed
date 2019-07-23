








































#define _BSD_SOURCE
#include "cairoint.h"

#if CAIRO_HAS_FONT_SUBSET

#include "cairo-type1-private.h"
#include "cairo-scaled-font-subsets-private.h"
#include "cairo-output-stream-private.h"


#if CAIRO_HAS_FT_FONT

#include "cairo-ft-private.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_TYPE1_TABLES_H

#include <ctype.h>

typedef struct _cairo_type1_font_subset {
    cairo_scaled_font_subset_t *scaled_font_subset;

    struct {
	cairo_unscaled_font_t *unscaled_font;
	unsigned int font_id;
	char *base_font;
	unsigned int num_glyphs;
	long x_min, y_min, x_max, y_max;
	long ascent, descent;

	const char    *data;
	unsigned long  header_size;
	unsigned long  data_size;
	unsigned long  trailer_size;
    } base;

    FT_Face face;
    int num_glyphs;

    struct {
	int subset_index;
	int width;
	char *name;
    } *glyphs;

    cairo_output_stream_t *output;
    cairo_array_t contents;

    const char *rd, *nd;

    char *type1_data;
    unsigned int type1_length;
    char *type1_end;

    char *header_segment;
    int header_segment_size;
    char *eexec_segment;
    int eexec_segment_size;
    cairo_bool_t eexec_segment_is_ascii;

    char *cleartext;
    char *cleartext_end;

    int header_size;

    unsigned short eexec_key;
    cairo_bool_t hex_encode;
    int hex_column;
} cairo_type1_font_subset_t;


static cairo_status_t
_cairo_type1_font_subset_init (cairo_type1_font_subset_t  *font,
			       cairo_unscaled_font_t      *unscaled_font,
			       cairo_bool_t                hex_encode)
{
    cairo_ft_unscaled_font_t *ft_unscaled_font;
    cairo_status_t status;
    FT_Face face;
    PS_FontInfoRec font_info;
    int i, j;

    ft_unscaled_font = (cairo_ft_unscaled_font_t *) unscaled_font;

    face = _cairo_ft_unscaled_font_lock_face (ft_unscaled_font);
    if (unlikely (face == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    if (FT_Get_PS_Font_Info(face, &font_info) != 0) {
	status = CAIRO_INT_STATUS_UNSUPPORTED;
        goto fail1;
    }

    
#if HAVE_FT_LOAD_SFNT_TABLE
    if (FT_IS_SFNT (face)) {
	status = CAIRO_INT_STATUS_UNSUPPORTED;
        goto fail1;
    }
#endif

    memset (font, 0, sizeof (*font));
    font->base.unscaled_font = _cairo_unscaled_font_reference (unscaled_font);
    font->base.num_glyphs = face->num_glyphs;
    font->base.x_min = face->bbox.xMin;
    font->base.y_min = face->bbox.yMin;
    font->base.x_max = face->bbox.xMax;
    font->base.y_max = face->bbox.yMax;
    font->base.ascent = face->ascender;
    font->base.descent = face->descender;

    if (face->family_name) {
	font->base.base_font = strdup (face->family_name);
	if (unlikely (font->base.base_font == NULL)) {
	    status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    goto fail2;
	}
	for (i = 0, j = 0; font->base.base_font[j]; j++) {
	    if (font->base.base_font[j] == ' ')
		continue;
	    font->base.base_font[i++] = font->base.base_font[j];
	}
	font->base.base_font[i] = '\0';
    }

    font->glyphs = calloc (face->num_glyphs, sizeof font->glyphs[0]);
    if (unlikely (font->glyphs == NULL)) {
        status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto fail3;
    }

    font->hex_encode = hex_encode;
    font->num_glyphs = 0;
    for (i = 0; i < face->num_glyphs; i++)
	font->glyphs[i].subset_index = -1;

    _cairo_array_init (&font->contents, sizeof (char));

    _cairo_ft_unscaled_font_unlock_face (ft_unscaled_font);

    return CAIRO_STATUS_SUCCESS;

 fail3:
    if (font->base.base_font)
	free (font->base.base_font);
 fail2:
    _cairo_unscaled_font_destroy (unscaled_font);
 fail1:
    _cairo_ft_unscaled_font_unlock_face (ft_unscaled_font);

    return status;
}

static void
cairo_type1_font_subset_use_glyph (cairo_type1_font_subset_t *font, int glyph)
{
    if (font->glyphs[glyph].subset_index >= 0)
	return;

    font->glyphs[glyph].subset_index = font->num_glyphs++;
}

static cairo_bool_t
is_ps_delimiter(int c)
{
    static const char delimiters[] = "()[]{}<>/% \t\r\n";

    return strchr (delimiters, c) != NULL;
}

static const char *
find_token (const char *buffer, const char *end, const char *token)
{
    int i, length;
    

    if (buffer == NULL)
	return NULL;

    length = strlen (token);
    for (i = 0; buffer + i < end - length + 1; i++)
	if (memcmp (buffer + i, token, length) == 0)
	    if ((i == 0 || token[0] == '/' || is_ps_delimiter(buffer[i - 1])) &&
		(buffer + i == end - length || is_ps_delimiter(buffer[i + length])))
		return buffer + i;

    return NULL;
}

static cairo_status_t
cairo_type1_font_subset_find_segments (cairo_type1_font_subset_t *font)
{
    unsigned char *p;
    const char *eexec_token;
    int size, i;

    p = (unsigned char *) font->type1_data;
    font->type1_end = font->type1_data + font->type1_length;
    if (p[0] == 0x80 && p[1] == 0x01) {
	font->header_segment_size =
	    p[2] | (p[3] << 8) | (p[4] << 16) | (p[5] << 24);
	font->header_segment = (char *) p + 6;

	p += 6 + font->header_segment_size;
	font->eexec_segment_size =
	    p[2] | (p[3] << 8) | (p[4] << 16) | (p[5] << 24);
	font->eexec_segment = (char *) p + 6;
	font->eexec_segment_is_ascii = (p[1] == 1);

        p += 6 + font->eexec_segment_size;
        while (p < (unsigned char *) (font->type1_end) && p[1] != 0x03) {
            size = p[2] | (p[3] << 8) | (p[4] << 16) | (p[5] << 24);
            p += 6 + size;
        }
        font->type1_end = (char *) p;
    } else {
	eexec_token = find_token ((char *) p, font->type1_end, "eexec");
	if (eexec_token == NULL)
	    return CAIRO_INT_STATUS_UNSUPPORTED;

	font->header_segment_size = eexec_token - (char *) p + strlen ("eexec\n");
	font->header_segment = (char *) p;
	font->eexec_segment_size = font->type1_length - font->header_segment_size;
	font->eexec_segment = (char *) p + font->header_segment_size;
	font->eexec_segment_is_ascii = TRUE;
	for (i = 0; i < 4; i++) {
	    if (!isxdigit(font->eexec_segment[i]))
		font->eexec_segment_is_ascii = FALSE;
	}
    }

    return CAIRO_STATUS_SUCCESS;
}










static void
cairo_type1_font_erase_dict_key (cairo_type1_font_subset_t *font,
				 const char *key)
{
    const char *start, *p, *segment_end;

    segment_end = font->header_segment + font->header_segment_size;

    start = font->header_segment;
    do {
	start = find_token (start, segment_end, key);
	if (start) {
	    p = start + strlen(key);
	    
	    while (p < segment_end &&
		   (_cairo_isspace(*p) ||
		    _cairo_isdigit(*p) ||
		    *p == '[' ||
		    *p == ']'))
	    {
		p++;
	    }

	    if (p + 3 < segment_end && memcmp(p, "def", 3) == 0) {
		
		memset((char *) start, ' ', p + 3 - start);
	    }
	    start += strlen(key);
	}
    } while (start);
}

static cairo_status_t
cairo_type1_font_subset_write_header (cairo_type1_font_subset_t *font,
					 const char *name)
{
    const char *start, *end, *segment_end;
    unsigned int i;

    















    cairo_type1_font_erase_dict_key (font, "/UniqueID");
    cairo_type1_font_erase_dict_key (font, "/XUID");

    segment_end = font->header_segment + font->header_segment_size;

    









    end = font->header_segment;
    start = find_token (font->header_segment, segment_end, "/UniqueID");
    if (start) {
	start += 9;
	while (start < segment_end && _cairo_isspace (*start))
	    start++;
	if (start + 5 < segment_end && memcmp(start, "known", 5) == 0) {
	    _cairo_output_stream_write (font->output, font->header_segment,
					start + 5 - font->header_segment);
	    _cairo_output_stream_printf (font->output, " pop false ");
	    end = start + 5;
	}
    }

    start = find_token (end, segment_end, "/FontName");
    if (start == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    _cairo_output_stream_write (font->output, end,
				start - end);

    _cairo_output_stream_printf (font->output, "/FontName /%s def", name);

    end = find_token (start, segment_end, "def");
    if (end == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;
    end += 3;

    start = find_token (end, segment_end, "/Encoding");
    if (start == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;
    _cairo_output_stream_write (font->output, end, start - end);

    _cairo_output_stream_printf (font->output,
				 "/Encoding 256 array\n"
				 "0 1 255 {1 index exch /.notdef put} for\n");
    for (i = 1; i < font->base.num_glyphs; i++) {
	if (font->glyphs[i].subset_index < 0)
	    continue;
	_cairo_output_stream_printf (font->output,
				     "dup %d /%s put\n",
				     font->glyphs[i].subset_index,
				     font->glyphs[i].name);
    }
    _cairo_output_stream_printf (font->output, "readonly def");

    end = find_token (start, segment_end, "def");
    if (end == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;
    end += 3;

    _cairo_output_stream_write (font->output, end, segment_end - end);

    return font->output->status;
}

static int
hex_to_int (int ch)
{
    if (ch <= '9')
	return ch - '0';
    else if (ch <= 'F')
	return ch - 'A' + 10;
    else
	return ch - 'a' + 10;
}

static cairo_status_t
cairo_type1_font_subset_write_encrypted (cairo_type1_font_subset_t *font,
					 const char *data, unsigned int length)
{
    const unsigned char *in, *end;
    int c, p;
    static const char hex_digits[16] = "0123456789abcdef";
    char digits[3];

    in = (const unsigned char *) data;
    end = (const unsigned char *) data + length;
    while (in < end) {
	p = *in++;
	c = p ^ (font->eexec_key >> 8);
	font->eexec_key = (c + font->eexec_key) * CAIRO_TYPE1_ENCRYPT_C1 + CAIRO_TYPE1_ENCRYPT_C2;

	if (font->hex_encode) {
	    digits[0] = hex_digits[c >> 4];
	    digits[1] = hex_digits[c & 0x0f];
	    digits[2] = '\n';
	    font->hex_column += 2;

	    if (font->hex_column == 78) {
		_cairo_output_stream_write (font->output, digits, 3);
		font->hex_column = 0;
	    } else {
		_cairo_output_stream_write (font->output, digits, 2);
	    }
	} else {
	    digits[0] = c;
	    _cairo_output_stream_write (font->output, digits, 1);
	}
    }

    return font->output->status;
}

static cairo_status_t
cairo_type1_font_subset_decrypt_eexec_segment (cairo_type1_font_subset_t *font)
{
    unsigned short r = CAIRO_TYPE1_PRIVATE_DICT_KEY;
    unsigned char *in, *end;
    char *out;
    int c, p;
    int i;

    in = (unsigned char *) font->eexec_segment;
    end = (unsigned char *) in + font->eexec_segment_size;

    font->cleartext = malloc (font->eexec_segment_size);
    if (unlikely (font->cleartext == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    out = font->cleartext;
    while (in < end) {
	if (font->eexec_segment_is_ascii) {
	    c = *in++;
	    if (_cairo_isspace (c))
		continue;
	    c = (hex_to_int (c) << 4) | hex_to_int (*in++);
	} else {
	    c = *in++;
	}
	p = c ^ (r >> 8);
	r = (c + r) * CAIRO_TYPE1_ENCRYPT_C1 + CAIRO_TYPE1_ENCRYPT_C2;

	*out++ = p;
    }
    font->cleartext_end = out;

    













    for (i = 0; i < 4 && i < font->eexec_segment_size; i++)
	font->cleartext[i] = ' ';

    return CAIRO_STATUS_SUCCESS;
}

static const char *
skip_token (const char *p, const char *end)
{
    while (p < end && _cairo_isspace(*p))
	p++;

    while (p < end && !_cairo_isspace(*p))
	p++;

    if (p == end)
	return NULL;

    return p;
}

static int
cairo_type1_font_subset_lookup_glyph (cairo_type1_font_subset_t *font,
				      const char *glyph_name, int length)
{
    unsigned int i;

    for (i = 0; i < font->base.num_glyphs; i++) {
	if (font->glyphs[i].name &&
	    strncmp (font->glyphs[i].name, glyph_name, length) == 0 &&
	    font->glyphs[i].name[length] == '\0')
	    return i;
    }

    return -1;
}

static cairo_status_t
cairo_type1_font_subset_get_glyph_names_and_widths (cairo_type1_font_subset_t *font)
{
    unsigned int i;
    char buffer[256];
    FT_Error error;

    
    for (i = 0; i < font->base.num_glyphs; i++) {
	if (font->glyphs[i].name != NULL)
	    continue;

	error = FT_Load_Glyph (font->face, i,
			       FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING |
			       FT_LOAD_NO_BITMAP | FT_LOAD_IGNORE_TRANSFORM);
	if (error != FT_Err_Ok) {
	    
	    if (error == FT_Err_Out_Of_Memory)
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	    return CAIRO_INT_STATUS_UNSUPPORTED;
	}

	font->glyphs[i].width = font->face->glyph->metrics.horiAdvance;

	error = FT_Get_Glyph_Name(font->face, i, buffer, sizeof buffer);
	if (error != FT_Err_Ok) {
	    
	    if (error == FT_Err_Out_Of_Memory)
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	    return CAIRO_INT_STATUS_UNSUPPORTED;
	}

	font->glyphs[i].name = strdup (buffer);
	if (unlikely (font->glyphs[i].name == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    return CAIRO_STATUS_SUCCESS;
}

static void
cairo_type1_font_subset_decrypt_charstring (const unsigned char *in, int size, unsigned char *out)
{
    unsigned short r = CAIRO_TYPE1_CHARSTRING_KEY;
    int c, p, i;

    for (i = 0; i < size; i++) {
        c = *in++;
	p = c ^ (r >> 8);
	r = (c + r) * CAIRO_TYPE1_ENCRYPT_C1 + CAIRO_TYPE1_ENCRYPT_C2;
	*out++ = p;
    }
}

static const unsigned char *
cairo_type1_font_subset_decode_integer (const unsigned char *p, int *integer)
{
    if (*p <= 246) {
        *integer = *p++ - 139;
    } else if (*p <= 250) {
        *integer = (p[0] - 247) * 256 + p[1] + 108;
        p += 2;
    } else if (*p <= 254) {
        *integer = -(p[0] - 251) * 256 - p[1] - 108;
        p += 2;
    } else {
        *integer = (p[1] << 24) | (p[2] << 16) | (p[3] << 8) | p[4];
        p += 5;
    }

    return p;
}

#if 0




@encoding = (
	
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	
	"space",	"exclam",	"quotedbl",	"numbersign",
	"dollar",	"percent",	"ampersand",	"quoteright",
	"parenleft",	"parenright",	"asterisk",	"plus",
	"comma",	"hyphen",	"period",	"slash",
	
	"zero",		"one",		"two",		"three",
	"four",		"five",		"six",		"seven",
	"eight",	"nine",		"colon",	"semicolon",
	"less",		"equal",	"greater",	"question",
	
	"at",		"A",		"B",		"C",
	"D",		"E",		"F",		"G",
	"H",		"I",		"J",		"K",
	"L",		"M",		"N",		"O",
	
	"P",		"Q",		"R",		"S",
	"T",		"U",		"V",		"W",
	"X",		"Y",		"Z",		"bracketleft",
	"backslash",	"bracketright",	"asciicircum",	"underscore",
	
	"quoteleft",	"a",		"b",		"c",
	"d",		"e",		"f",		"g",
	"h",		"i",		"j",		"k",
	"l",		"m",		"n",		"o",
	
	"p",		"q",		"r",		"s",
	"t",		"u",		"v",		"w",
	"x",		"y",		"z",		"braceleft",
	"bar",		"braceright",	"asciitilde",	NULL,
	
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	
	NULL,		"exclamdown",	"cent",		"sterling",
	"fraction",	"yen",		"florin",	"section",
	"currency",	"quotesingle",	"quotedblleft",	"guillemotleft",
	"guilsinglleft","guilsinglright","fi",		"fl",
	
	NULL,		"endash",	"dagger",	"daggerdbl",
	"periodcentered",NULL,		"paragraph",	"bullet",
	"quotesinglbase","quotedblbase","quotedblright","guillemotright",
	"ellipsis",	"perthousand",	NULL,		"questiondown",
	
	NULL,		"grave",	"acute",	"circumflex",
	"tilde",	"macron",	"breve",	"dotaccent",
	"dieresis",	NULL,		"ring",		"cedilla",
	NULL,		"hungarumlaut",	"ogonek",	"caron",
	
	"emdash",	NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	
	NULL,		"AE",		NULL,		"ordfeminine",
	NULL,		NULL,		NULL,		NULL,
	"Lslash",	"Oslash",	"OE",		"ordmasculine",
	NULL,		NULL,		NULL,		NULL,
	
	NULL,		"ae",		NULL,		NULL,
	NULL,		"dotlessi",	NULL,		NULL,
	"lslash",	"oslash",	"oe",		"germandbls",
	NULL,		NULL,		NULL,		NULL
	);

print "static const char ps_standard_encoding_symbol[] = {\n";
$s = qq( "\\0");
for $sym (@encoding) {
    if (! ($sym eq NULL)) {
        $ss = qq( "$sym\\0");
	if (length($s) + length($ss) > 78) {
	  print qq( $s\n);
	  $s = "";
	}
	$s .= $ss;
    }
}
print qq( $s\n);
print "};\n\n";
print "static const int16_t ps_standard_encoding_offset[256] = {\n";
$offset = 1;
$s = qq();
for $sym (@encoding) {
    if (! ($sym eq NULL)) {
	$ss = qq( $offset,);
	$offset += length($sym) + 1;
    } else {
	$ss = qq( 0,);
    }
    if (length($s) + length($ss) > 78) {
      print qq( $s\n);
      $s = "";
    }
    $s .= $ss;
}
print qq( $s\n);
print "};\n";
exit;
#endif

static const char ps_standard_encoding_symbol[] = {
  "\0" "space\0" "exclam\0" "quotedbl\0" "numbersign\0" "dollar\0" "percent\0"
  "ampersand\0" "quoteright\0" "parenleft\0" "parenright\0" "asterisk\0"
  "plus\0" "comma\0" "hyphen\0" "period\0" "slash\0" "zero\0" "one\0" "two\0"
  "three\0" "four\0" "five\0" "six\0" "seven\0" "eight\0" "nine\0" "colon\0"
  "semicolon\0" "less\0" "equal\0" "greater\0" "question\0" "at\0" "A\0" "B\0"
  "C\0" "D\0" "E\0" "F\0" "G\0" "H\0" "I\0" "J\0" "K\0" "L\0" "M\0" "N\0" "O\0"
  "P\0" "Q\0" "R\0" "S\0" "T\0" "U\0" "V\0" "W\0" "X\0" "Y\0" "Z\0"
  "bracketleft\0" "backslash\0" "bracketright\0" "asciicircum\0" "underscore\0"
  "quoteleft\0" "a\0" "b\0" "c\0" "d\0" "e\0" "f\0" "g\0" "h\0" "i\0" "j\0"
  "k\0" "l\0" "m\0" "n\0" "o\0" "p\0" "q\0" "r\0" "s\0" "t\0" "u\0" "v\0" "w\0"
  "x\0" "y\0" "z\0" "braceleft\0" "bar\0" "braceright\0" "asciitilde\0"
  "exclamdown\0" "cent\0" "sterling\0" "fraction\0" "yen\0" "florin\0"
  "section\0" "currency\0" "quotesingle\0" "quotedblleft\0" "guillemotleft\0"
  "guilsinglleft\0" "guilsinglright\0" "fi\0" "fl\0" "endash\0" "dagger\0"
  "daggerdbl\0" "periodcentered\0" "paragraph\0" "bullet\0" "quotesinglbase\0"
  "quotedblbase\0" "quotedblright\0" "guillemotright\0" "ellipsis\0"
  "perthousand\0" "questiondown\0" "grave\0" "acute\0" "circumflex\0" "tilde\0"
  "macron\0" "breve\0" "dotaccent\0" "dieresis\0" "ring\0" "cedilla\0"
  "hungarumlaut\0" "ogonek\0" "caron\0" "emdash\0" "AE\0" "ordfeminine\0"
  "Lslash\0" "Oslash\0" "OE\0" "ordmasculine\0" "ae\0" "dotlessi\0" "lslash\0"
  "oslash\0" "oe\0" "germandbls\0"
};

static const int16_t ps_standard_encoding_offset[256] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 7, 14, 23,
  34, 41, 49, 59,
  70, 80, 91, 100, 105,
  111, 118, 125, 131, 136,
  140, 144, 150, 155, 160, 164,
  170, 176, 181, 187, 197,
  202, 208, 216, 225, 228, 230,
  232, 234, 236, 238, 240, 242, 244,
  246, 248, 250, 252, 254, 256, 258,
  260, 262, 264, 266, 268, 270, 272,
  274, 276, 278, 280, 292,
  302, 315, 327, 338,
  348, 350, 352, 354, 356, 358, 360,
  362, 364, 366, 368, 370, 372, 374,
  376, 378, 380, 382, 384, 386, 388,
  390, 392, 394, 396, 398, 400,
  410, 414, 425, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  436, 447, 452, 461, 470,
  474, 481, 489, 498,
  510, 523, 537,
  551, 566, 569, 0, 572, 579,
  586, 596, 0, 611, 621,
  628, 643, 656,
  670, 685, 694, 0,
  706, 0, 719, 725, 731,
  742, 748, 755, 761, 771,
  0, 780, 785, 0, 793, 806,
  813, 819, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  826, 0, 829, 0, 0, 0, 0, 841, 848,
  855, 858, 0, 0, 0, 0, 0, 871, 0, 0, 0,
  874, 0, 0, 883, 890, 897,
  900, 0, 0, 0, 0,
};

#define ps_standard_encoding(index) ((index) ? ps_standard_encoding_symbol+ps_standard_encoding_offset[(index)] : NULL)

static cairo_status_t
use_standard_encoding_glyph (cairo_type1_font_subset_t *font, int index)
{
    const char *glyph_name;

    if (index < 0 || index > 255)
	return CAIRO_STATUS_SUCCESS;

    glyph_name = ps_standard_encoding(index);
    if (glyph_name == NULL)
	return CAIRO_STATUS_SUCCESS;

    index = cairo_type1_font_subset_lookup_glyph (font,
						  glyph_name,
						  strlen(glyph_name));
    if (index < 0)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    cairo_type1_font_subset_use_glyph (font, index);

    return CAIRO_STATUS_SUCCESS;
}

#define TYPE1_CHARSTRING_COMMAND_ESCAPE		(12)
#define TYPE1_CHARSTRING_COMMAND_SEAC		(32 + 6)

static cairo_status_t
cairo_type1_font_subset_look_for_seac(cairo_type1_font_subset_t *font,
				      const char *name, int name_length,
				      const char *encrypted_charstring, int encrypted_charstring_length)
{
    cairo_status_t status;
    unsigned char *charstring;
    const unsigned char *end;
    const unsigned char *p;
    int stack[5], sp, value;
    int command;

    charstring = malloc (encrypted_charstring_length);
    if (unlikely (charstring == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    cairo_type1_font_subset_decrypt_charstring ((const unsigned char *)
						encrypted_charstring,
						encrypted_charstring_length,
						charstring);
    end = charstring + encrypted_charstring_length;

    p = charstring + 4;
    sp = 0;

    while (p < end) {
        if (*p < 32) {
	    command = *p++;

	    if (command == TYPE1_CHARSTRING_COMMAND_ESCAPE)
		command = 32 + *p++;

	    switch (command) {
	    case TYPE1_CHARSTRING_COMMAND_SEAC:
		





		status = use_standard_encoding_glyph (font, stack[3]);
		if (unlikely (status))
		    return status;

		status = use_standard_encoding_glyph (font, stack[4]);
		if (unlikely (status))
		    return status;

		sp = 0;
		break;

	    default:
		sp = 0;
		break;
	    }
        } else {
            
	    p = cairo_type1_font_subset_decode_integer (p, &value);
	    if (sp < 5)
		stack[sp++] = value;
        }
    }

    free (charstring);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
write_used_glyphs (cairo_type1_font_subset_t *font,
		   const char *name, int name_length,
		   const char *charstring, int charstring_length)
{
    cairo_status_t status;
    char buffer[256];
    int length;

    length = snprintf (buffer, sizeof buffer,
		       "/%.*s %d %s ",
		       name_length, name, charstring_length, font->rd);
    status = cairo_type1_font_subset_write_encrypted (font, buffer, length);
    if (unlikely (status))
	return status;

    status = cairo_type1_font_subset_write_encrypted (font,
					              charstring,
						      charstring_length);
    if (unlikely (status))
	return status;

    length = snprintf (buffer, sizeof buffer, "%s\n", font->nd);
    status = cairo_type1_font_subset_write_encrypted (font, buffer, length);
    if (unlikely (status))
	return status;

    return CAIRO_STATUS_SUCCESS;
}

typedef cairo_status_t (*glyph_func_t) (cairo_type1_font_subset_t *font,
			                const char *name, int name_length,
			                const char *charstring, int charstring_length);

static cairo_status_t
cairo_type1_font_subset_for_each_glyph (cairo_type1_font_subset_t *font,
					const char *dict_start,
					const char *dict_end,
					glyph_func_t func,
					const char **dict_out)
{
    int charstring_length, name_length, glyph_index;
    const char *p, *charstring, *name;
    char *end;

    













    p = dict_start;

    while (*p == '/') {
	name = p + 1;
	p = skip_token (p, dict_end);
	name_length = p - name;

	charstring_length = strtol (p, &end, 10);
	if (p == end)
	    return CAIRO_INT_STATUS_UNSUPPORTED;

	

	charstring = skip_token (end, dict_end) + 1;

	
	p = skip_token (charstring + charstring_length, dict_end);
	while (p < dict_end && _cairo_isspace(*p))
	    p++;

	

	if (p == dict_end)
	    return CAIRO_INT_STATUS_UNSUPPORTED;

	glyph_index = cairo_type1_font_subset_lookup_glyph (font,
							    name, name_length);
	if (font->glyphs[glyph_index].subset_index >= 0) {
	    cairo_status_t status = func (font,
		                          name, name_length,
					  charstring, charstring_length);
	    if (unlikely (status))
		return status;
	}
    }

    *dict_out = p;

    return CAIRO_STATUS_SUCCESS;
}


static cairo_status_t
cairo_type1_font_subset_write_private_dict (cairo_type1_font_subset_t *font,
					    const char                *name)
{
    cairo_status_t status;
    const char *p, *charstrings, *dict_start;
    const char *closefile_token;
    char buffer[32], *glyph_count_end;
    int num_charstrings, length;

    












    


    charstrings = find_token (font->cleartext, font->cleartext_end, "/CharStrings");
    if (charstrings == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    
    p = charstrings + strlen ("/CharStrings");
    num_charstrings = strtol (p, &glyph_count_end, 10);
    if (p == glyph_count_end)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    

    for (p = glyph_count_end; p < font->cleartext_end; p++)
	if (*p == '/')
	    break;
    if (p == font->cleartext_end)
	return CAIRO_INT_STATUS_UNSUPPORTED;
    dict_start = p;

    status = cairo_type1_font_subset_get_glyph_names_and_widths (font);
    if (unlikely (status))
	return status;

    



    status = cairo_type1_font_subset_for_each_glyph (font,
						     dict_start,
						     font->cleartext_end,
						     cairo_type1_font_subset_look_for_seac,
						     &p);
    if (unlikely (status))
	return status;

    closefile_token = find_token (p, font->cleartext_end, "closefile");
    if (closefile_token == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    status = cairo_type1_font_subset_get_glyph_names_and_widths (font);
    if (unlikely (status))
	return status;

    

    status = cairo_type1_font_subset_write_header (font, name);
    if (unlikely (status))
	return status;

    font->base.header_size = _cairo_output_stream_get_position (font->output);


    

    status = cairo_type1_font_subset_write_encrypted (font, font->cleartext,
					         charstrings - font->cleartext);
    if (unlikely (status))
	return status;

    
    length = snprintf (buffer, sizeof buffer,
		       "/CharStrings %d", font->num_glyphs);
    status = cairo_type1_font_subset_write_encrypted (font, buffer, length);
    if (unlikely (status))
	return status;

    

    status = cairo_type1_font_subset_write_encrypted (font, glyph_count_end,
	                                          dict_start - glyph_count_end);
    if (unlikely (status))
	return status;

    

    status = cairo_type1_font_subset_for_each_glyph (font,
						     dict_start,
						     font->cleartext_end,
						     write_used_glyphs,
						     &p);
    if (unlikely (status))
	return status;

    

    status = cairo_type1_font_subset_write_encrypted (font, p,
	                        closefile_token - p + strlen ("closefile") + 1);
    if (unlikely (status))
	return status;

    if (font->hex_encode)
	_cairo_output_stream_write (font->output, "\n", 1);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
cairo_type1_font_subset_write_trailer(cairo_type1_font_subset_t *font)
{
    const char *cleartomark_token;
    int i;
    static const char zeros[65] =
	"0000000000000000000000000000000000000000000000000000000000000000\n";


    for (i = 0; i < 8; i++)
	_cairo_output_stream_write (font->output, zeros, sizeof zeros);

    cleartomark_token = find_token (font->type1_data, font->type1_end, "cleartomark");
    if (cleartomark_token) {
	



	_cairo_output_stream_write (font->output, cleartomark_token,
				    font->type1_end - cleartomark_token);
    } else if (!font->eexec_segment_is_ascii) {
	



	_cairo_output_stream_printf (font->output, "cleartomark");
    } else {
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    
    _cairo_output_stream_printf (font->output, "\n");

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
type1_font_write (void *closure, const unsigned char *data, unsigned int length)
{
    cairo_type1_font_subset_t *font = closure;

    return _cairo_array_append_multiple (&font->contents, data, length);
}

static cairo_status_t
cairo_type1_font_subset_write (cairo_type1_font_subset_t *font,
			       const char *name)
{
    cairo_status_t status;

    status = cairo_type1_font_subset_find_segments (font);
    if (unlikely (status))
	return status;

    status = cairo_type1_font_subset_decrypt_eexec_segment (font);
    if (unlikely (status))
	return status;

    
    if (find_token (font->cleartext, font->cleartext_end, "/-|") != NULL) {
	font->rd = "-|";
	font->nd = "|-";
    } else if (find_token (font->cleartext, font->cleartext_end, "/RD") != NULL) {
	font->rd = "RD";
	font->nd = "ND";
    } else {
	
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    font->eexec_key = CAIRO_TYPE1_PRIVATE_DICT_KEY;
    font->hex_column = 0;

    status = cairo_type1_font_subset_write_private_dict (font, name);
    if (unlikely (status))
	return status;

    font->base.data_size = _cairo_output_stream_get_position (font->output) -
	font->base.header_size;

    status = cairo_type1_font_subset_write_trailer (font);
    if (unlikely (status))
	return status;

    font->base.trailer_size =
	_cairo_output_stream_get_position (font->output) -
	font->base.header_size - font->base.data_size;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
cairo_type1_font_subset_generate (void       *abstract_font,
				  const char *name)

{
    cairo_type1_font_subset_t *font = abstract_font;
    cairo_ft_unscaled_font_t *ft_unscaled_font;
    unsigned long ret;
    cairo_status_t status;

    ft_unscaled_font = (cairo_ft_unscaled_font_t *) font->base.unscaled_font;
    font->face = _cairo_ft_unscaled_font_lock_face (ft_unscaled_font);
    if (unlikely (font->face == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    font->type1_length = font->face->stream->size;
    font->type1_data = malloc (font->type1_length);
    if (unlikely (font->type1_data == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto fail;
    }

    if (font->face->stream->read != NULL) {
	




	ret = (* font->face->stream->read) (font->face->stream, 0,
					    (unsigned char *) font->type1_data,
					    font->type1_length);
	if (ret != font->type1_length) {
	    status = _cairo_error (CAIRO_STATUS_READ_ERROR);
	    goto fail;
	}
    } else {
	memcpy (font->type1_data,
		font->face->stream->base, font->type1_length);
    }

    status = _cairo_array_grow_by (&font->contents, 4096);
    if (unlikely (status))
	goto fail;

    font->output = _cairo_output_stream_create (type1_font_write, NULL, font);
    if (unlikely ((status = font->output->status)))
	goto fail;

    status = cairo_type1_font_subset_write (font, name);
    if (unlikely (status))
	goto fail;

    font->base.data = _cairo_array_index (&font->contents, 0);

 fail:
    _cairo_ft_unscaled_font_unlock_face (ft_unscaled_font);

    return status;
}

static cairo_status_t
_cairo_type1_font_subset_fini (cairo_type1_font_subset_t *font)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    unsigned int i;

    


    _cairo_array_fini (&font->contents);

    free (font->type1_data);
    if (font->glyphs != NULL) {
	for (i = 0; i < font->base.num_glyphs; i++)
	    free (font->glyphs[i].name);
    }

    _cairo_unscaled_font_destroy (font->base.unscaled_font);

    if (font->output != NULL)
	status = _cairo_output_stream_destroy (font->output);

    if (font->base.base_font)
	free (font->base.base_font);
    free (font->glyphs);

    return status;
}

cairo_status_t
_cairo_type1_subset_init (cairo_type1_subset_t		*type1_subset,
			  const char			*name,
			  cairo_scaled_font_subset_t	*scaled_font_subset,
                          cairo_bool_t                   hex_encode)
{
    cairo_type1_font_subset_t font;
    cairo_status_t status, status_ignored;
    unsigned long parent_glyph, length;
    unsigned int i;
    cairo_unscaled_font_t *unscaled_font;
    char buf[30];

    
    if (!_cairo_scaled_font_is_ft (scaled_font_subset->scaled_font))
	return CAIRO_INT_STATUS_UNSUPPORTED;

    if (_cairo_ft_scaled_font_is_vertical (scaled_font_subset->scaled_font))
        return CAIRO_INT_STATUS_UNSUPPORTED;

    unscaled_font = _cairo_ft_scaled_font_get_unscaled_font (scaled_font_subset->scaled_font);

    status = _cairo_type1_font_subset_init (&font, unscaled_font, hex_encode);
    if (unlikely (status))
	return status;

    for (i = 0; i < scaled_font_subset->num_glyphs; i++) {
	parent_glyph = scaled_font_subset->glyphs[i];
	cairo_type1_font_subset_use_glyph (&font, parent_glyph);
    }

    status = cairo_type1_font_subset_generate (&font, name);
    if (unlikely (status))
	goto fail1;

    if (font.base.base_font) {
	type1_subset->base_font = strdup (font.base.base_font);
    } else {
        snprintf(buf, sizeof (buf), "CairoFont-%u-%u",
                 scaled_font_subset->font_id, scaled_font_subset->subset_id);
	type1_subset->base_font = strdup (buf);
    }
    if (unlikely (type1_subset->base_font == NULL))
	goto fail1;

    type1_subset->widths = calloc (sizeof (int), font.num_glyphs);
    if (unlikely (type1_subset->widths == NULL))
	goto fail2;
    for (i = 0; i < font.base.num_glyphs; i++) {
	if (font.glyphs[i].subset_index < 0)
	    continue;
	type1_subset->widths[font.glyphs[i].subset_index] =
	    font.glyphs[i].width;
    }

    type1_subset->x_min = font.base.x_min;
    type1_subset->y_min = font.base.y_min;
    type1_subset->x_max = font.base.x_max;
    type1_subset->y_max = font.base.y_max;
    type1_subset->ascent = font.base.ascent;
    type1_subset->descent = font.base.descent;

    length = font.base.header_size +
	     font.base.data_size +
	     font.base.trailer_size;
    type1_subset->data = malloc (length);
    if (unlikely (type1_subset->data == NULL))
	goto fail3;

    memcpy (type1_subset->data,
	    _cairo_array_index (&font.contents, 0), length);

    type1_subset->header_length = font.base.header_size;
    type1_subset->data_length = font.base.data_size;
    type1_subset->trailer_length = font.base.trailer_size;

    return _cairo_type1_font_subset_fini (&font);

 fail3:
    free (type1_subset->widths);
 fail2:
    free (type1_subset->base_font);
 fail1:
    status_ignored = _cairo_type1_font_subset_fini (&font);

    return status;
}

void
_cairo_type1_subset_fini (cairo_type1_subset_t *subset)
{
    free (subset->base_font);
    free (subset->widths);
    free (subset->data);
}

cairo_bool_t
_cairo_type1_scaled_font_is_type1 (cairo_scaled_font_t *scaled_font)
{
    cairo_ft_unscaled_font_t *unscaled;
    FT_Face face;
    PS_FontInfoRec font_info;
    cairo_bool_t is_type1 = FALSE;

    if (!_cairo_scaled_font_is_ft (scaled_font))
       return FALSE;
    unscaled = (cairo_ft_unscaled_font_t *) _cairo_ft_scaled_font_get_unscaled_font (scaled_font);
    face = _cairo_ft_unscaled_font_lock_face (unscaled);
    if (!face)
        return FALSE;

    if (FT_Get_PS_Font_Info(face, &font_info) == 0)
        is_type1 = TRUE;

    
#if HAVE_FT_LOAD_SFNT_TABLE
    if (FT_IS_SFNT (face))
        is_type1 = FALSE;
#endif

    _cairo_ft_unscaled_font_unlock_face (unscaled);

    return is_type1;
}

#endif 

#endif 
