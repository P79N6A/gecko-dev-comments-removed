










































































#ifndef CSS_PROP_SHORTHAND
#define CSS_PROP_SHORTHAND(name_, id_, method_, flags_, pref_)
#define DEFINED_CSS_PROP_SHORTHAND
#endif

#define CSS_PROP_DOMPROP_PREFIXED(name_) \
  CSS_PROP_PUBLIC_OR_PRIVATE(Moz ## name_, name_)

#define CSS_PROP_NO_OFFSET (-1)
















#ifdef CSS_PROP

#define USED_CSS_PROP
#define CSS_PROP_FONT(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, Font, stylestructoffset_, animtype_)
#define CSS_PROP_COLOR(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, Color, stylestructoffset_, animtype_)
#define CSS_PROP_BACKGROUND(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, Background, stylestructoffset_, animtype_)
#define CSS_PROP_LIST(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, List, stylestructoffset_, animtype_)
#define CSS_PROP_POSITION(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, Position, stylestructoffset_, animtype_)
#define CSS_PROP_TEXT(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, Text, stylestructoffset_, animtype_)
#define CSS_PROP_TEXTRESET(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, TextReset, stylestructoffset_, animtype_)
#define CSS_PROP_DISPLAY(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, Display, stylestructoffset_, animtype_)
#define CSS_PROP_VISIBILITY(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, Visibility, stylestructoffset_, animtype_)
#define CSS_PROP_CONTENT(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, Content, stylestructoffset_, animtype_)
#define CSS_PROP_QUOTES(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, Quotes, stylestructoffset_, animtype_)
#define CSS_PROP_USERINTERFACE(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, UserInterface, stylestructoffset_, animtype_)
#define CSS_PROP_UIRESET(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, UIReset, stylestructoffset_, animtype_)
#define CSS_PROP_TABLE(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, Table, stylestructoffset_, animtype_)
#define CSS_PROP_TABLEBORDER(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, TableBorder, stylestructoffset_, animtype_)
#define CSS_PROP_MARGIN(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, Margin, stylestructoffset_, animtype_)
#define CSS_PROP_PADDING(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, Padding, stylestructoffset_, animtype_)
#define CSS_PROP_BORDER(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, Border, stylestructoffset_, animtype_)
#define CSS_PROP_OUTLINE(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, Outline, stylestructoffset_, animtype_)
#define CSS_PROP_XUL(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, XUL, stylestructoffset_, animtype_)
#define CSS_PROP_COLUMN(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, Column, stylestructoffset_, animtype_)
#define CSS_PROP_SVG(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, SVG, stylestructoffset_, animtype_)
#define CSS_PROP_SVGRESET(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, SVGReset, stylestructoffset_, animtype_)
#define CSS_PROP_VARIABLES(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, Variables, stylestructoffset_, animtype_)




#ifndef CSS_PROP_BACKENDONLY
#define CSS_PROP_BACKENDONLY(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_) CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, BackendOnly, CSS_PROP_NO_OFFSET, eStyleAnimType_None)
#define DEFINED_CSS_PROP_BACKENDONLY
#endif

#else 





#ifndef CSS_PROP_FONT
#define CSS_PROP_FONT(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_FONT
#endif
#ifndef CSS_PROP_COLOR
#define CSS_PROP_COLOR(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_COLOR
#endif
#ifndef CSS_PROP_BACKGROUND
#define CSS_PROP_BACKGROUND(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_BACKGROUND
#endif
#ifndef CSS_PROP_LIST
#define CSS_PROP_LIST(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_LIST
#endif
#ifndef CSS_PROP_POSITION
#define CSS_PROP_POSITION(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_POSITION
#endif
#ifndef CSS_PROP_TEXT
#define CSS_PROP_TEXT(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_TEXT
#endif
#ifndef CSS_PROP_TEXTRESET
#define CSS_PROP_TEXTRESET(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_TEXTRESET
#endif
#ifndef CSS_PROP_DISPLAY
#define CSS_PROP_DISPLAY(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_DISPLAY
#endif
#ifndef CSS_PROP_VISIBILITY
#define CSS_PROP_VISIBILITY(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_VISIBILITY
#endif
#ifndef CSS_PROP_CONTENT
#define CSS_PROP_CONTENT(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_CONTENT
#endif
#ifndef CSS_PROP_QUOTES
#define CSS_PROP_QUOTES(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_QUOTES
#endif
#ifndef CSS_PROP_USERINTERFACE
#define CSS_PROP_USERINTERFACE(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_USERINTERFACE
#endif
#ifndef CSS_PROP_UIRESET
#define CSS_PROP_UIRESET(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_UIRESET
#endif
#ifndef CSS_PROP_TABLE
#define CSS_PROP_TABLE(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_TABLE
#endif
#ifndef CSS_PROP_TABLEBORDER
#define CSS_PROP_TABLEBORDER(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_TABLEBORDER
#endif
#ifndef CSS_PROP_MARGIN
#define CSS_PROP_MARGIN(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_MARGIN
#endif
#ifndef CSS_PROP_PADDING
#define CSS_PROP_PADDING(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_PADDING
#endif
#ifndef CSS_PROP_BORDER
#define CSS_PROP_BORDER(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_BORDER
#endif
#ifndef CSS_PROP_OUTLINE
#define CSS_PROP_OUTLINE(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_OUTLINE
#endif
#ifndef CSS_PROP_XUL
#define CSS_PROP_XUL(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_XUL
#endif
#ifndef CSS_PROP_COLUMN
#define CSS_PROP_COLUMN(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_COLUMN
#endif
#ifndef CSS_PROP_SVG
#define CSS_PROP_SVG(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_SVG
#endif
#ifndef CSS_PROP_SVGRESET
#define CSS_PROP_SVGRESET(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_SVGRESET
#endif
#ifndef CSS_PROP_VARIABLES
#define CSS_PROP_VARIABLES(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_, stylestructoffset_, animtype_)
#define DEFINED_CSS_PROP_VARIABLES
#endif

#ifndef CSS_PROP_BACKENDONLY
#define CSS_PROP_BACKENDONLY(name_, id_, method_, flags_, pref_, parsevariant_, kwtable_)
#define DEFINED_CSS_PROP_BACKENDONLY
#endif

#endif 































CSS_PROP_DISPLAY(
    -moz-appearance,
    appearance,
    CSS_PROP_DOMPROP_PREFIXED(Appearance),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kAppearanceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SHORTHAND(
    -moz-outline-radius,
    _moz_outline_radius,
    CSS_PROP_DOMPROP_PREFIXED(OutlineRadius),
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_OUTLINE(
    -moz-outline-radius-topleft,
    _moz_outline_radius_topLeft,
    CSS_PROP_DOMPROP_PREFIXED(OutlineRadiusTopleft),
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    0,
    nullptr,
    offsetof(nsStyleOutline, mOutlineRadius),
    eStyleAnimType_Corner_TopLeft)
CSS_PROP_OUTLINE(
    -moz-outline-radius-topright,
    _moz_outline_radius_topRight,
    CSS_PROP_DOMPROP_PREFIXED(OutlineRadiusTopright),
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    0,
    nullptr,
    offsetof(nsStyleOutline, mOutlineRadius),
    eStyleAnimType_Corner_TopRight)
CSS_PROP_OUTLINE(
    -moz-outline-radius-bottomright,
    _moz_outline_radius_bottomRight,
    CSS_PROP_DOMPROP_PREFIXED(OutlineRadiusBottomright),
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    0,
    nullptr,
    offsetof(nsStyleOutline, mOutlineRadius),
    eStyleAnimType_Corner_BottomRight)
CSS_PROP_OUTLINE(
    -moz-outline-radius-bottomleft,
    _moz_outline_radius_bottomLeft,
    CSS_PROP_DOMPROP_PREFIXED(OutlineRadiusBottomleft),
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    0,
    nullptr,
    offsetof(nsStyleOutline, mOutlineRadius),
    eStyleAnimType_Corner_BottomLeft)
CSS_PROP_TEXT(
    -moz-tab-size,
    _moz_tab_size,
    CSS_PROP_DOMPROP_PREFIXED(TabSize),
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE,
    "",
    VARIANT_HI,
    nullptr,
    offsetof(nsStyleText, mTabSize),
    eStyleAnimType_None)
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_FONT(
    -x-system-font,
    _x_system_font,
    CSS_PROP_DOMPROP_PREFIXED(SystemFont),
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    0,
    kFontKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif 
CSS_PROP_SHORTHAND(
    all,
    all,
    All,
    CSS_PROPERTY_PARSE_FUNCTION,
    "layout.css.all-shorthand.enabled")
CSS_PROP_SHORTHAND(
    animation,
    animation,
    Animation,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_DISPLAY(
    animation-delay,
    animation_delay,
    AnimationDelay,
    CSS_PROPERTY_PARSE_VALUE_LIST |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS,
    "",
    VARIANT_TIME, 
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_DISPLAY(
    animation-direction,
    animation_direction,
    AnimationDirection,
    CSS_PROPERTY_PARSE_VALUE_LIST |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS,
    "",
    VARIANT_KEYWORD, 
    kAnimationDirectionKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_DISPLAY(
    animation-duration,
    animation_duration,
    AnimationDuration,
    CSS_PROPERTY_PARSE_VALUE_LIST |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS,
    "",
    VARIANT_TIME | VARIANT_NONNEGATIVE_DIMENSION, 
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_DISPLAY(
    animation-fill-mode,
    animation_fill_mode,
    AnimationFillMode,
    CSS_PROPERTY_PARSE_VALUE_LIST |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS,
    "",
    VARIANT_KEYWORD, 
    kAnimationFillModeKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_DISPLAY(
    animation-iteration-count,
    animation_iteration_count,
    AnimationIterationCount,
    CSS_PROPERTY_PARSE_VALUE_LIST |
        
        
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS,
    "",
    VARIANT_KEYWORD | VARIANT_NUMBER, 
    kAnimationIterationCountKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_DISPLAY(
    animation-name,
    animation_name,
    AnimationName,
    CSS_PROPERTY_PARSE_VALUE_LIST |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS,
    "",
    
    
    VARIANT_NONE | VARIANT_IDENTIFIER_NO_INHERIT, 
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_DISPLAY(
    animation-play-state,
    animation_play_state,
    AnimationPlayState,
    CSS_PROPERTY_PARSE_VALUE_LIST |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS,
    "",
    VARIANT_KEYWORD, 
    kAnimationPlayStateKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_DISPLAY(
    animation-timing-function,
    animation_timing_function,
    AnimationTimingFunction,
    CSS_PROPERTY_PARSE_VALUE_LIST |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS,
    "",
    VARIANT_KEYWORD | VARIANT_TIMING_FUNCTION, 
    kTransitionTimingFunctionKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SHORTHAND(
    background,
    background,
    Background,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_BACKGROUND(
    background-attachment,
    background_attachment,
    BackgroundAttachment,
    CSS_PROPERTY_PARSE_VALUE_LIST |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS,
    "",
    VARIANT_KEYWORD, 
    kBackgroundAttachmentKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BACKGROUND(
    background-clip,
    background_clip,
    BackgroundClip,
    CSS_PROPERTY_PARSE_VALUE_LIST |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS,
    "",
    VARIANT_KEYWORD, 
    kBackgroundOriginKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BACKGROUND(
    background-color,
    background_color,
    BackgroundColor,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED |
        CSS_PROPERTY_HASHLESS_COLOR_QUIRK,
    "",
    VARIANT_HC,
    nullptr,
    offsetof(nsStyleBackground, mBackgroundColor),
    eStyleAnimType_Color)
CSS_PROP_BACKGROUND(
    background-image,
    background_image,
    BackgroundImage,
    CSS_PROPERTY_PARSE_VALUE_LIST |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED |
        CSS_PROPERTY_START_IMAGE_LOADS,
    "",
    VARIANT_IMAGE, 
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BACKGROUND(
    background-blend-mode,
    background_blend_mode,
    BackgroundBlendMode,
    CSS_PROPERTY_PARSE_VALUE_LIST |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS,
    "layout.css.background-blend-mode.enabled",
    VARIANT_KEYWORD, 
    kBlendModeKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BACKGROUND(
    background-origin,
    background_origin,
    BackgroundOrigin,
    CSS_PROPERTY_PARSE_VALUE_LIST |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS,
    "",
    VARIANT_KEYWORD, 
    kBackgroundOriginKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BACKGROUND(
    background-position,
    background_position,
    BackgroundPosition,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS |
        CSS_PROPERTY_STORES_CALC,
    "",
    0,
    kBackgroundPositionKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_BACKGROUND(
    background-repeat,
    background_repeat,
    BackgroundRepeat,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS,
    "",
    VARIANT_KEYWORD, 
    kBackgroundRepeatKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BACKGROUND(
    background-size,
    background_size,
    BackgroundSize,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_STORES_CALC,
    "",
    0,
    kBackgroundSizeKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_DISPLAY(
    -moz-binding,
    binding,
    CSS_PROP_DOMPROP_PREFIXED(Binding),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HUO,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None) 
CSS_PROP_SHORTHAND(
    border,
    border,
    Border,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_SHORTHAND(
    border-bottom,
    border_bottom,
    BorderBottom,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_BORDER(
    border-bottom-color,
    border_bottom_color,
    BorderBottomColor,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED |
        CSS_PROPERTY_HASHLESS_COLOR_QUIRK,
    "",
    VARIANT_HCK,
    kBorderColorKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_BORDER(
    -moz-border-bottom-colors,
    border_bottom_colors,
    CSS_PROP_DOMPROP_PREFIXED(BorderBottomColors),
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BORDER(
    border-bottom-style,
    border_bottom_style,
    BorderBottomStyle,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER,
    "",
    VARIANT_HK,
    kBorderStyleKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)  
CSS_PROP_BORDER(
    border-bottom-width,
    border_bottom_width,
    BorderBottomWidth,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_HKL | VARIANT_CALC,
    kBorderWidthKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_TABLEBORDER(
    border-collapse,
    border_collapse,
    BorderCollapse,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kBorderCollapseKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SHORTHAND(
    border-color,
    border_color,
    BorderColor,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_HASHLESS_COLOR_QUIRK,
    "")
CSS_PROP_SHORTHAND(
    -moz-border-end,
    border_end,
    CSS_PROP_DOMPROP_PREFIXED(BorderEnd),
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_SHORTHAND(
    -moz-border-end-color,
    border_end_color,
    CSS_PROP_DOMPROP_PREFIXED(BorderEndColor),
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_BORDER(
    border-end-color-value,
    border_end_color_value,
    BorderEndColorValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER,
    "",
    VARIANT_HCK, 
    kBorderColorKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_SHORTHAND(
    -moz-border-end-style,
    border_end_style,
    CSS_PROP_DOMPROP_PREFIXED(BorderEndStyle),
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_BORDER(
    border-end-style-value,
    border_end_style_value,
    BorderEndStyleValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER,
    "",
    VARIANT_HK, 
    kBorderStyleKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_SHORTHAND(
    -moz-border-end-width,
    border_end_width,
    CSS_PROP_DOMPROP_PREFIXED(BorderEndWidth),
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_BORDER(
    border-end-width-value,
    border_end_width_value,
    BorderEndWidthValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER,
    "",
    VARIANT_HKL | VARIANT_CALC,
    kBorderWidthKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_SHORTHAND(
    border-image,
    border_image,
    BorderImage,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_BORDER(
    border-image-source,
    border_image_source,
    BorderImageSource,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_START_IMAGE_LOADS,
    "",
    VARIANT_IMAGE | VARIANT_INHERIT,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BORDER(
    border-image-slice,
    border_image_slice,
    BorderImageSlice,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER,
    "",
    0,
    kBorderImageSliceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BORDER(
    border-image-width,
    border_image_width,
    BorderImageWidth,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BORDER(
    border-image-outset,
    border_image_outset,
    BorderImageOutset,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BORDER(
    border-image-repeat,
    border_image_repeat,
    BorderImageRepeat,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER,
    "",
    0,
    kBorderImageRepeatKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SHORTHAND(
    border-left,
    border_left,
    BorderLeft,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_SHORTHAND(
    border-left-color,
    border_left_color,
    BorderLeftColor,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_HASHLESS_COLOR_QUIRK,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_BORDER(
    border-left-color-value,
    border_left_color_value,
    BorderLeftColorValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED |
        CSS_PROPERTY_REPORT_OTHER_NAME,
    "",
    VARIANT_HCK, 
    kBorderColorKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_BORDER(
    border-left-color-ltr-source,
    border_left_color_ltr_source,
    BorderLeftColorLTRSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BORDER(
    border-left-color-rtl-source,
    border_left_color_rtl_source,
    BorderLeftColorRTLSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_BORDER(
    -moz-border-left-colors,
    border_left_colors,
    CSS_PROP_DOMPROP_PREFIXED(BorderLeftColors),
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SHORTHAND(
    border-left-style,
    border_left_style,
    BorderLeftStyle,
    CSS_PROPERTY_PARSE_FUNCTION,
    "") 
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_BORDER(
    border-left-style-value,
    border_left_style_value,
    BorderLeftStyleValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_REPORT_OTHER_NAME,
    "",
    VARIANT_HK, 
    kBorderStyleKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BORDER(
    border-left-style-ltr-source,
    border_left_style_ltr_source,
    BorderLeftStyleLTRSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BORDER(
    border-left-style-rtl-source,
    border_left_style_rtl_source,
    BorderLeftStyleRTLSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_SHORTHAND(
    border-left-width,
    border_left_width,
    BorderLeftWidth,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_BORDER(
    border-left-width-value,
    border_left_width_value,
    BorderLeftWidthValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_REPORT_OTHER_NAME,
    "",
    VARIANT_HKL | VARIANT_CALC,
    kBorderWidthKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_BORDER(
    border-left-width-ltr-source,
    border_left_width_ltr_source,
    BorderLeftWidthLTRSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BORDER(
    border-left-width-rtl-source,
    border_left_width_rtl_source,
    BorderLeftWidthRTLSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_SHORTHAND(
    border-right,
    border_right,
    BorderRight,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_SHORTHAND(
    border-right-color,
    border_right_color,
    BorderRightColor,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_HASHLESS_COLOR_QUIRK,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_BORDER(
    border-right-color-value,
    border_right_color_value,
    BorderRightColorValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED |
        CSS_PROPERTY_REPORT_OTHER_NAME,
    "",
    VARIANT_HCK, 
    kBorderColorKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_BORDER(
    border-right-color-ltr-source,
    border_right_color_ltr_source,
    BorderRightColorLTRSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BORDER(
    border-right-color-rtl-source,
    border_right_color_rtl_source,
    BorderRightColorRTLSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_BORDER(
    -moz-border-right-colors,
    border_right_colors,
    CSS_PROP_DOMPROP_PREFIXED(BorderRightColors),
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SHORTHAND(
    border-right-style,
    border_right_style,
    BorderRightStyle,
    CSS_PROPERTY_PARSE_FUNCTION,
    "") 
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_BORDER(
    border-right-style-value,
    border_right_style_value,
    BorderRightStyleValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_REPORT_OTHER_NAME,
    "",
    VARIANT_HK, 
    kBorderStyleKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BORDER(
    border-right-style-ltr-source,
    border_right_style_ltr_source,
    BorderRightStyleLTRSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BORDER(
    border-right-style-rtl-source,
    border_right_style_rtl_source,
    BorderRightStyleRTLSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_SHORTHAND(
    border-right-width,
    border_right_width,
    BorderRightWidth,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_BORDER(
    border-right-width-value,
    border_right_width_value,
    BorderRightWidthValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_REPORT_OTHER_NAME,
    "",
    VARIANT_HKL | VARIANT_CALC,
    kBorderWidthKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_BORDER(
    border-right-width-ltr-source,
    border_right_width_ltr_source,
    BorderRightWidthLTRSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BORDER(
    border-right-width-rtl-source,
    border_right_width_rtl_source,
    BorderRightWidthRTLSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_TABLEBORDER(
    border-spacing,
    border_spacing,
    BorderSpacing,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_VALUE_NONNEGATIVE,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_SHORTHAND(
    -moz-border-start,
    border_start,
    CSS_PROP_DOMPROP_PREFIXED(BorderStart),
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_SHORTHAND(
    -moz-border-start-color,
    border_start_color,
    CSS_PROP_DOMPROP_PREFIXED(BorderStartColor),
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_BORDER(
    border-start-color-value,
    border_start_color_value,
    BorderStartColorValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER,
    "",
    VARIANT_HCK, 
    kBorderColorKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_SHORTHAND(
    -moz-border-start-style,
    border_start_style,
    CSS_PROP_DOMPROP_PREFIXED(BorderStartStyle),
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_BORDER(
    border-start-style-value,
    border_start_style_value,
    BorderStartStyleValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER,
    "",
    VARIANT_HK, 
    kBorderStyleKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_SHORTHAND(
    -moz-border-start-width,
    border_start_width,
    CSS_PROP_DOMPROP_PREFIXED(BorderStartWidth),
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_BORDER(
    border-start-width-value,
    border_start_width_value,
    BorderStartWidthValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER,
    "",
    VARIANT_HKL | VARIANT_CALC,
    kBorderWidthKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_SHORTHAND(
    border-style,
    border_style,
    BorderStyle,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")  
CSS_PROP_SHORTHAND(
    border-top,
    border_top,
    BorderTop,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_BORDER(
    border-top-color,
    border_top_color,
    BorderTopColor,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED |
        CSS_PROPERTY_HASHLESS_COLOR_QUIRK,
    "",
    VARIANT_HCK,
    kBorderColorKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_BORDER(
    -moz-border-top-colors,
    border_top_colors,
    CSS_PROP_DOMPROP_PREFIXED(BorderTopColors),
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BORDER(
    border-top-style,
    border_top_style,
    BorderTopStyle,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER,
    "",
    VARIANT_HK,
    kBorderStyleKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)  
CSS_PROP_BORDER(
    border-top-width,
    border_top_width,
    BorderTopWidth,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_HKL | VARIANT_CALC,
    kBorderWidthKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_SHORTHAND(
    border-width,
    border_width,
    BorderWidth,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK,
    "")
CSS_PROP_SHORTHAND(
    border-radius,
    border_radius,
    BorderRadius,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_BORDER(
    border-top-left-radius,
    border_top_left_radius,
    BorderTopLeftRadius,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    0,
    nullptr,
    offsetof(nsStyleBorder, mBorderRadius),
    eStyleAnimType_Corner_TopLeft)
CSS_PROP_BORDER(
    border-top-right-radius,
    border_top_right_radius,
    BorderTopRightRadius,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    0,
    nullptr,
    offsetof(nsStyleBorder, mBorderRadius),
    eStyleAnimType_Corner_TopRight)
CSS_PROP_BORDER(
    border-bottom-right-radius,
    border_bottom_right_radius,
    BorderBottomRightRadius,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    0,
    nullptr,
    offsetof(nsStyleBorder, mBorderRadius),
    eStyleAnimType_Corner_BottomRight)
CSS_PROP_BORDER(
    border-bottom-left-radius,
    border_bottom_left_radius,
    BorderBottomLeftRadius,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    0,
    nullptr,
    offsetof(nsStyleBorder, mBorderRadius),
    eStyleAnimType_Corner_BottomLeft)
CSS_PROP_POSITION(
    bottom,
    bottom,
    Bottom,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_AHLP | VARIANT_CALC,
    nullptr,
    offsetof(nsStylePosition, mOffset),
    eStyleAnimType_Sides_Bottom)
CSS_PROP_BORDER(
    box-decoration-break,
    box_decoration_break,
    BoxDecorationBreak,
    CSS_PROPERTY_PARSE_VALUE,
    "layout.css.box-decoration-break.enabled",
    VARIANT_HK,
    kBoxDecorationBreakKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BORDER(
    box-shadow,
    box_shadow,
    BoxShadow,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED,
        
    "",
    0,
    kBoxShadowTypeKTable,
    offsetof(nsStyleBorder, mBoxShadow),
    eStyleAnimType_Shadow)
CSS_PROP_POSITION(
    box-sizing,
    box_sizing,
    BoxSizing,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kBoxSizingKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_TABLEBORDER(
    caption-side,
    caption_side,
    CaptionSide,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kCaptionSideKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_DISPLAY(
    clear,
    clear,
    Clear,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kClearKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_DISPLAY(
    clip,
    clip,
    Clip,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK,
    "",
    0,
    nullptr,
    offsetof(nsStyleDisplay, mClip),
    eStyleAnimType_Custom)
CSS_PROP_COLOR(
    color,
    color,
    Color,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED |
        CSS_PROPERTY_HASHLESS_COLOR_QUIRK,
    "",
    VARIANT_HC,
    nullptr,
    offsetof(nsStyleColor, mColor),
    eStyleAnimType_Color)
CSS_PROP_SHORTHAND(
    -moz-columns,
    _moz_columns,
    CSS_PROP_DOMPROP_PREFIXED(Columns),
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_COLUMN(
    -moz-column-count,
    _moz_column_count,
    CSS_PROP_DOMPROP_PREFIXED(ColumnCount),
    CSS_PROPERTY_PARSE_VALUE |
        
        
        CSS_PROPERTY_VALUE_AT_LEAST_ONE,
    "",
    VARIANT_AHI,
    nullptr,
    offsetof(nsStyleColumn, mColumnCount),
    eStyleAnimType_Custom)
CSS_PROP_COLUMN(
    -moz-column-fill,
    _moz_column_fill,
    CSS_PROP_DOMPROP_PREFIXED(ColumnFill),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kColumnFillKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_COLUMN(
    -moz-column-width,
    _moz_column_width,
    CSS_PROP_DOMPROP_PREFIXED(ColumnWidth),
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE,
    "",
    VARIANT_AHL | VARIANT_CALC,
    nullptr,
    offsetof(nsStyleColumn, mColumnWidth),
    eStyleAnimType_Coord)
CSS_PROP_COLUMN(
    -moz-column-gap,
    _moz_column_gap,
    CSS_PROP_DOMPROP_PREFIXED(ColumnGap),
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE,
    "",
    VARIANT_HL | VARIANT_NORMAL | VARIANT_CALC,
    nullptr,
    offsetof(nsStyleColumn, mColumnGap),
    eStyleAnimType_Coord)
CSS_PROP_SHORTHAND(
    -moz-column-rule,
    _moz_column_rule,
    CSS_PROP_DOMPROP_PREFIXED(ColumnRule),
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_COLUMN(
    -moz-column-rule-color,
    _moz_column_rule_color,
    CSS_PROP_DOMPROP_PREFIXED(ColumnRuleColor),
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED,
    "",
    VARIANT_HCK,
    kBorderColorKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_COLUMN(
    -moz-column-rule-style,
    _moz_column_rule_style,
    CSS_PROP_DOMPROP_PREFIXED(ColumnRuleStyle),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kBorderStyleKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_COLUMN(
    -moz-column-rule-width,
    _moz_column_rule_width,
    CSS_PROP_DOMPROP_PREFIXED(ColumnRuleWidth),
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE,
    "",
    VARIANT_HKL | VARIANT_CALC,
    kBorderWidthKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_CONTENT(
    content,
    content,
    Content,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_START_IMAGE_LOADS,
    "",
    0,
    kContentKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_TEXT(
    -moz-control-character-visibility,
    _moz_control_character_visibility,
    CSS_PROP_DOMPROP_PREFIXED(ControlCharacterVisibility),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kControlCharacterVisibilityKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_CONTENT(
    counter-increment,
    counter_increment,
    CounterIncrement,
    CSS_PROPERTY_PARSE_FUNCTION,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None) 
CSS_PROP_CONTENT(
    counter-reset,
    counter_reset,
    CounterReset,
    CSS_PROPERTY_PARSE_FUNCTION,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None) 
CSS_PROP_USERINTERFACE(
    cursor,
    cursor,
    Cursor,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS |
        CSS_PROPERTY_START_IMAGE_LOADS |
        CSS_PROPERTY_IMAGE_IS_IN_ARRAY_0,
    "",
    0,
    kCursorKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#ifndef CSS_PROP_LIST_ONLY_COMPONENTS_OF_ALL_SHORTHAND
CSS_PROP_VISIBILITY(
    direction,
    direction,
    Direction,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kDirectionKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif 
CSS_PROP_DISPLAY(
    display,
    display,
    Display,
    CSS_PROPERTY_PARSE_VALUE |
        
        
        
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    VARIANT_HK,
    kDisplayKTable,
    offsetof(nsStyleDisplay, mDisplay),
    eStyleAnimType_EnumU8)
CSS_PROP_TABLEBORDER(
    empty-cells,
    empty_cells,
    EmptyCells,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kEmptyCellsKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_POSITION(
    align-content,
    align_content,
    AlignContent,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kAlignContentKTable,
    offsetof(nsStylePosition, mAlignContent),
    eStyleAnimType_EnumU8)
CSS_PROP_POSITION(
    align-items,
    align_items,
    AlignItems,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kAlignItemsKTable,
    offsetof(nsStylePosition, mAlignItems),
    eStyleAnimType_EnumU8)
CSS_PROP_POSITION(
    align-self,
    align_self,
    AlignSelf,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kAlignSelfKTable,
    offsetof(nsStylePosition, mAlignSelf),
    eStyleAnimType_EnumU8)
CSS_PROP_SHORTHAND(
    flex,
    flex,
    Flex,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_POSITION(
    flex-basis,
    flex_basis,
    FlexBasis,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_STORES_CALC,
    "",
    
    
    
    VARIANT_AHKLP | VARIANT_CALC,
    kFlexBasisKTable,
    offsetof(nsStylePosition, mFlexBasis),
    eStyleAnimType_Coord)
CSS_PROP_POSITION(
    flex-direction,
    flex_direction,
    FlexDirection,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kFlexDirectionKTable,
    offsetof(nsStylePosition, mFlexDirection),
    eStyleAnimType_EnumU8)
CSS_PROP_SHORTHAND(
    flex-flow,
    flex_flow,
    FlexFlow,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_POSITION(
    flex-grow,
    flex_grow,
    FlexGrow,
    CSS_PROPERTY_PARSE_VALUE |
      CSS_PROPERTY_VALUE_NONNEGATIVE,
    "",
    
    
    
    VARIANT_HN,
    nullptr,
    offsetof(nsStylePosition, mFlexGrow),
    eStyleAnimType_float)
CSS_PROP_POSITION(
    flex-shrink,
    flex_shrink,
    FlexShrink,
    CSS_PROPERTY_PARSE_VALUE |
      CSS_PROPERTY_VALUE_NONNEGATIVE,
    "",
    
    
    
    VARIANT_HN,
    nullptr,
    offsetof(nsStylePosition, mFlexShrink),
    eStyleAnimType_float)
CSS_PROP_POSITION(
    flex-wrap,
    flex_wrap,
    FlexWrap,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kFlexWrapKTable,
    offsetof(nsStylePosition, mFlexWrap),
    eStyleAnimType_EnumU8)
CSS_PROP_POSITION(
    order,
    order,
    Order,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HI,
    nullptr,
    offsetof(nsStylePosition, mOrder),
    eStyleAnimType_Custom) 
CSS_PROP_POSITION(
    justify-content,
    justify_content,
    JustifyContent,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kJustifyContentKTable,
    offsetof(nsStylePosition, mJustifyContent),
    eStyleAnimType_EnumU8)
CSS_PROP_DISPLAY(
    float,
    float,
    CSS_PROP_PUBLIC_OR_PRIVATE(CssFloat, Float),
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER,
    "",
    VARIANT_HK,
    kFloatKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BORDER(
    -moz-float-edge,
    float_edge,
    CSS_PROP_DOMPROP_PREFIXED(FloatEdge),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kFloatEdgeKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None) 
CSS_PROP_SHORTHAND(
    font,
    font,
    Font,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_FONT(
    font-family,
    font_family,
    FontFamily,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_PARSER_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_FONT(
    font-feature-settings,
    font_feature_settings,
    FontFeatureSettings,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_PARSER_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_FONT(
    font-kerning,
    font_kerning,
    FontKerning,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    VARIANT_HK,
    kFontKerningKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_FONT(
    font-language-override,
    font_language_override,
    FontLanguageOverride,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    VARIANT_NORMAL | VARIANT_INHERIT | VARIANT_STRING,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_FONT(
    font-size,
    font_size,
    FontSize,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK,
    "",
    VARIANT_HKLP | VARIANT_SYSFONT | VARIANT_CALC,
    kFontSizeKTable,
    
    
    offsetof(nsStyleFont, mSize),
    eStyleAnimType_nscoord)
CSS_PROP_FONT(
    font-size-adjust,
    font_size_adjust,
    FontSizeAdjust,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    VARIANT_HON | VARIANT_SYSFONT,
    nullptr,
    offsetof(nsStyleFont, mFont.sizeAdjust),
    eStyleAnimType_float)
CSS_PROP_FONT(
    -moz-osx-font-smoothing,
    osx_font_smoothing,
    CSS_PROP_DOMPROP_PREFIXED(OSXFontSmoothing),
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "layout.css.osx-font-smoothing.enabled",
    VARIANT_HK,
    kFontSmoothingKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_FONT(
    font-stretch,
    font_stretch,
    FontStretch,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    VARIANT_HK | VARIANT_SYSFONT,
    kFontStretchKTable,
    offsetof(nsStyleFont, mFont.stretch),
    eStyleAnimType_Custom)
CSS_PROP_FONT(
    font-style,
    font_style,
    FontStyle,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    VARIANT_HK | VARIANT_SYSFONT,
    kFontStyleKTable,
    offsetof(nsStyleFont, mFont.style),
    eStyleAnimType_EnumU8)
CSS_PROP_FONT(
    font-synthesis,
    font_synthesis,
    FontSynthesis,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_PARSER_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    0,
    kFontSynthesisKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SHORTHAND(
    font-variant,
    font_variant,
    FontVariant,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_FONT(
    font-variant-alternates,
    font_variant_alternates,
    FontVariantAlternates,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_PARSER_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    0,
    kFontVariantAlternatesKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_FONT(
    font-variant-caps,
    font_variant_caps,
    FontVariantCaps,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    VARIANT_HMK,
    kFontVariantCapsKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_FONT(
    font-variant-east-asian,
    font_variant_east_asian,
    FontVariantEastAsian,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_PARSER_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    0,
    kFontVariantEastAsianKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_FONT(
    font-variant-ligatures,
    font_variant_ligatures,
    FontVariantLigatures,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_PARSER_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    0,
    kFontVariantLigaturesKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_FONT(
    font-variant-numeric,
    font_variant_numeric,
    FontVariantNumeric,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_PARSER_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    0,
    kFontVariantNumericKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_FONT(
    font-variant-position,
    font_variant_position,
    FontVariantPosition,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    VARIANT_HMK,
    kFontVariantPositionKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_FONT(
    font-weight,
    font_weight,
    FontWeight,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_PARSER_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
        
    "",
    0,
    kFontWeightKTable,
    offsetof(nsStyleFont, mFont.weight),
    eStyleAnimType_Custom)
CSS_PROP_UIRESET(
    -moz-force-broken-image-icon,
    force_broken_image_icon,
    CSS_PROP_DOMPROP_PREFIXED(ForceBrokenImageIcon),
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE,
    "",
    VARIANT_HI,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None) 
CSS_PROP_POSITION(
    grid-auto-flow,
    grid_auto_flow,
    GridAutoFlow,
    CSS_PROPERTY_PARSE_FUNCTION,
    "layout.css.grid.enabled",
    0,
    kGridAutoFlowKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_POSITION(
    grid-auto-columns,
    grid_auto_columns,
    GridAutoColumns,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_STORES_CALC,
    "layout.css.grid.enabled",
    0,
    kGridTrackBreadthKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_POSITION(
    grid-auto-rows,
    grid_auto_rows,
    GridAutoRows,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_STORES_CALC,
    "layout.css.grid.enabled",
    0,
    kGridTrackBreadthKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_POSITION(
    grid-template-areas,
    grid_template_areas,
    GridTemplateAreas,
    CSS_PROPERTY_PARSE_FUNCTION,
    "layout.css.grid.enabled",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_POSITION(
    grid-template-columns,
    grid_template_columns,
    GridTemplateColumns,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "layout.css.grid.enabled",
    0,
    kGridTrackBreadthKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_POSITION(
    grid-template-rows,
    grid_template_rows,
    GridTemplateRows,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "layout.css.grid.enabled",
    0,
    kGridTrackBreadthKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SHORTHAND(
    grid-template,
    grid_template,
    GridTemplate,
    CSS_PROPERTY_PARSE_FUNCTION,
    "layout.css.grid.enabled")
CSS_PROP_SHORTHAND(
    grid,
    grid,
    Grid,
    CSS_PROPERTY_PARSE_FUNCTION,
    "layout.css.grid.enabled")
CSS_PROP_POSITION(
    grid-column-start,
    grid_column_start,
    GridColumnStart,
    CSS_PROPERTY_PARSE_FUNCTION,
    "layout.css.grid.enabled",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_POSITION(
    grid-column-end,
    grid_column_end,
    GridColumnEnd,
    CSS_PROPERTY_PARSE_FUNCTION,
    "layout.css.grid.enabled",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_POSITION(
    grid-row-start,
    grid_row_start,
    GridRowStart,
    CSS_PROPERTY_PARSE_FUNCTION,
    "layout.css.grid.enabled",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_POSITION(
    grid-row-end,
    grid_row_end,
    GridRowEnd,
    CSS_PROPERTY_PARSE_FUNCTION,
    "layout.css.grid.enabled",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SHORTHAND(
    grid-column,
    grid_column,
    GridColumn,
    CSS_PROPERTY_PARSE_FUNCTION,
    "layout.css.grid.enabled")
CSS_PROP_SHORTHAND(
    grid-row,
    grid_row,
    GridRow,
    CSS_PROPERTY_PARSE_FUNCTION,
    "layout.css.grid.enabled")
CSS_PROP_SHORTHAND(
    grid-area,
    grid_area,
    GridArea,
    CSS_PROPERTY_PARSE_FUNCTION,
    "layout.css.grid.enabled")
CSS_PROP_POSITION(
    height,
    height,
    Height,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_AHLP | VARIANT_CALC,
    nullptr,
    offsetof(nsStylePosition, mHeight),
    eStyleAnimType_Coord)
CSS_PROP_VISIBILITY(
    image-orientation,
    image_orientation,
    ImageOrientation,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_PARSER_FUNCTION,
    "layout.css.image-orientation.enabled",
    0,
    kImageOrientationKTable,
    offsetof(nsStyleVisibility, mImageOrientation),
    eStyleAnimType_None)
CSS_PROP_LIST(
    -moz-image-region,
    image_region,
    CSS_PROP_DOMPROP_PREFIXED(ImageRegion),
    CSS_PROPERTY_PARSE_FUNCTION,
    "",
    0,
    nullptr,
    offsetof(nsStyleList, mImageRegion),
    eStyleAnimType_Custom)
CSS_PROP_UIRESET(
    ime-mode,
    ime_mode,
    ImeMode,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kIMEModeKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_POSITION(
    left,
    left,
    Left,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_AHLP | VARIANT_CALC,
    nullptr,
    offsetof(nsStylePosition, mOffset),
    eStyleAnimType_Sides_Left)
CSS_PROP_TEXT(
    letter-spacing,
    letter_spacing,
    LetterSpacing,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK,
    "",
    VARIANT_HL | VARIANT_NORMAL | VARIANT_CALC,
    nullptr,
    offsetof(nsStyleText, mLetterSpacing),
    eStyleAnimType_Coord)
CSS_PROP_TEXT(
    line-height,
    line_height,
    LineHeight,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_HLPN | VARIANT_KEYWORD | VARIANT_NORMAL | VARIANT_SYSFONT,
    kLineHeightKTable,
    offsetof(nsStyleText, mLineHeight),
    eStyleAnimType_Coord)
CSS_PROP_SHORTHAND(
    list-style,
    list_style,
    ListStyle,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_LIST(
    list-style-image,
    list_style_image,
    ListStyleImage,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_START_IMAGE_LOADS,
    "",
    VARIANT_HUO,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_LIST(
    list-style-position,
    list_style_position,
    ListStylePosition,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kListStylePositionKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_LIST(
    list-style-type,
    list_style_type,
    ListStyleType,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_PARSER_FUNCTION,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SHORTHAND(
    margin,
    margin,
    Margin,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_APPLIES_TO_PAGE_RULE,
    "")
CSS_PROP_MARGIN(
    margin-bottom,
    margin_bottom,
    MarginBottom,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_APPLIES_TO_PAGE_RULE |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_AHLP | VARIANT_CALC,
    nullptr,
    offsetof(nsStyleMargin, mMargin),
    eStyleAnimType_Sides_Bottom)
CSS_PROP_SHORTHAND(
    -moz-margin-end,
    margin_end,
    CSS_PROP_DOMPROP_PREFIXED(MarginEnd),
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_PAGE_RULE,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_MARGIN(
    margin-end-value,
    margin_end_value,
    MarginEndValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_APPLIES_TO_PAGE_RULE,
    "",
    VARIANT_AHLP | VARIANT_CALC, 
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_SHORTHAND(
    margin-left,
    margin_left,
    MarginLeft,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_APPLIES_TO_PAGE_RULE |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_MARGIN(
    margin-left-value,
    margin_left_value,
    MarginLeftValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_REPORT_OTHER_NAME |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_APPLIES_TO_PAGE_RULE,
    "",
    VARIANT_AHLP | VARIANT_CALC, 
    nullptr,
    offsetof(nsStyleMargin, mMargin),
    eStyleAnimType_Sides_Left)
CSS_PROP_MARGIN(
    margin-left-ltr-source,
    margin_left_ltr_source,
    MarginLeftLTRSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE |
        CSS_PROPERTY_APPLIES_TO_PAGE_RULE,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_MARGIN(
    margin-left-rtl-source,
    margin_left_rtl_source,
    MarginLeftRTLSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE |
        CSS_PROPERTY_APPLIES_TO_PAGE_RULE,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_SHORTHAND(
    margin-right,
    margin_right,
    MarginRight,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_APPLIES_TO_PAGE_RULE |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_MARGIN(
    margin-right-value,
    margin_right_value,
    MarginRightValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_REPORT_OTHER_NAME |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_APPLIES_TO_PAGE_RULE,
    "",
    VARIANT_AHLP | VARIANT_CALC, 
    nullptr,
    offsetof(nsStyleMargin, mMargin),
    eStyleAnimType_Sides_Right)
CSS_PROP_MARGIN(
    margin-right-ltr-source,
    margin_right_ltr_source,
    MarginRightLTRSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE |
        CSS_PROPERTY_APPLIES_TO_PAGE_RULE,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_MARGIN(
    margin-right-rtl-source,
    margin_right_rtl_source,
    MarginRightRTLSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE |
        CSS_PROPERTY_APPLIES_TO_PAGE_RULE,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_SHORTHAND(
    -moz-margin-start,
    margin_start,
    CSS_PROP_DOMPROP_PREFIXED(MarginStart),
    CSS_PROPERTY_PARSE_FUNCTION |
    CSS_PROPERTY_APPLIES_TO_PAGE_RULE,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_MARGIN(
    margin-start-value,
    margin_start_value,
    MarginStartValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_APPLIES_TO_PAGE_RULE,
    "",
    VARIANT_AHLP | VARIANT_CALC, 
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_MARGIN(
    margin-top,
    margin_top,
    MarginTop,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_APPLIES_TO_PAGE_RULE |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_AHLP | VARIANT_CALC,
    nullptr,
    offsetof(nsStyleMargin, mMargin),
    eStyleAnimType_Sides_Top)
CSS_PROP_CONTENT(
    marker-offset,
    marker_offset,
    MarkerOffset,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_AHL | VARIANT_CALC,
    nullptr,
    offsetof(nsStyleContent, mMarkerOffset),
    eStyleAnimType_Coord)
CSS_PROP_BACKENDONLY(
    marks,
    marks,
    Marks,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_PARSER_FUNCTION,
    "",
    0,
    kPageMarksKTable)
CSS_PROP_POSITION(
    max-height,
    max_height,
    MaxHeight,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_HLPO | VARIANT_CALC,
    nullptr,
    offsetof(nsStylePosition, mMaxHeight),
    eStyleAnimType_Coord)
CSS_PROP_POSITION(
    max-width,
    max_width,
    MaxWidth,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_HKLPO | VARIANT_CALC,
    kWidthKTable,
    offsetof(nsStylePosition, mMaxWidth),
    eStyleAnimType_Coord)
CSS_PROP_POSITION(
    min-height,
    min_height,
    MinHeight,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_AHLP | VARIANT_CALC,
    nullptr,
    offsetof(nsStylePosition, mMinHeight),
    eStyleAnimType_Coord)
CSS_PROP_POSITION(
    min-width,
    min_width,
    MinWidth,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_AHKLP | VARIANT_CALC,
    kWidthKTable,
    offsetof(nsStylePosition, mMinWidth),
    eStyleAnimType_Coord)
CSS_PROP_DISPLAY(
    mix-blend-mode,
    mix_blend_mode,
    MixBlendMode,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_CREATES_STACKING_CONTEXT,
    "layout.css.mix-blend-mode.enabled",
    VARIANT_HK,
    kBlendModeKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_POSITION(
    object-fit,
    object_fit,
    ObjectFit,
    CSS_PROPERTY_PARSE_VALUE,
    "layout.css.object-fit-and-position.enabled",
    VARIANT_HK,
    kObjectFitKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_POSITION(
    object-position,
    object_position,
    ObjectPosition,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_STORES_CALC,
    "layout.css.object-fit-and-position.enabled",
    0,
    kBackgroundPositionKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None) 
CSS_PROP_DISPLAY(
    opacity,
    opacity,
    Opacity,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    VARIANT_HN,
    nullptr,
    offsetof(nsStyleDisplay, mOpacity),
    eStyleAnimType_float)
CSS_PROP_DISPLAY(
    -moz-orient,
    orient,
    CSS_PROP_DOMPROP_PREFIXED(Orient),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kOrientKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BACKENDONLY(
    orphans,
    orphans,
    Orphans,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_AT_LEAST_ONE,
    "",
    VARIANT_HI,
    nullptr)
CSS_PROP_SHORTHAND(
    outline,
    outline,
    Outline,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_OUTLINE(
    outline-color,
    outline_color,
    OutlineColor,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED,
    "",
    VARIANT_HCK,
    kOutlineColorKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_OUTLINE(
    outline-style,
    outline_style,
    OutlineStyle,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kOutlineStyleKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_OUTLINE(
    outline-width,
    outline_width,
    OutlineWidth,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE,
    "",
    VARIANT_HKL | VARIANT_CALC,
    kBorderWidthKTable,
    offsetof(nsStyleOutline, mOutlineWidth),
    eStyleAnimType_Coord)
CSS_PROP_OUTLINE(
    outline-offset,
    outline_offset,
    OutlineOffset,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HL | VARIANT_CALC,
    nullptr,
    offsetof(nsStyleOutline, mOutlineOffset),
    eStyleAnimType_nscoord)
CSS_PROP_SHORTHAND(
    overflow,
    overflow,
    Overflow,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_DISPLAY(
    overflow-clip-box,
    overflow_clip_box,
    OverflowClipBox,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_ALWAYS_ENABLED_IN_UA_SHEETS |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "layout.css.overflow-clip-box.enabled",
    VARIANT_HK,
    kOverflowClipBoxKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_DISPLAY(
    overflow-x,
    overflow_x,
    OverflowX,
    CSS_PROPERTY_PARSE_VALUE |
        
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    VARIANT_HK,
    kOverflowSubKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_DISPLAY(
    overflow-y,
    overflow_y,
    OverflowY,
    CSS_PROPERTY_PARSE_VALUE |
        
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    VARIANT_HK,
    kOverflowSubKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SHORTHAND(
    padding,
    padding,
    Padding,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK,
    "")
CSS_PROP_PADDING(
    padding-bottom,
    padding_bottom,
    PaddingBottom,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_HLP | VARIANT_CALC,
    nullptr,
    offsetof(nsStylePadding, mPadding),
    eStyleAnimType_Sides_Bottom)
CSS_PROP_SHORTHAND(
    -moz-padding-end,
    padding_end,
    CSS_PROP_DOMPROP_PREFIXED(PaddingEnd),
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_PADDING(
    padding-end-value,
    padding_end_value,
    PaddingEndValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_STORES_CALC,
    "",
    VARIANT_HLP | VARIANT_CALC, 
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_SHORTHAND(
    padding-left,
    padding_left,
    PaddingLeft,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_PADDING(
    padding-left-value,
    padding_left_value,
    PaddingLeftValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_REPORT_OTHER_NAME |
        CSS_PROPERTY_STORES_CALC,
    "",
    VARIANT_HLP | VARIANT_CALC, 
    nullptr,
    offsetof(nsStylePadding, mPadding),
    eStyleAnimType_Sides_Left)
CSS_PROP_PADDING(
    padding-left-ltr-source,
    padding_left_ltr_source,
    PaddingLeftLTRSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_PADDING(
    padding-left-rtl-source,
    padding_left_rtl_source,
    PaddingLeftRTLSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_SHORTHAND(
    padding-right,
    padding_right,
    PaddingRight,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_PADDING(
    padding-right-value,
    padding_right_value,
    PaddingRightValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_REPORT_OTHER_NAME |
        CSS_PROPERTY_STORES_CALC,
    "",
    VARIANT_HLP | VARIANT_CALC, 
    nullptr,
    offsetof(nsStylePadding, mPadding),
    eStyleAnimType_Sides_Right)
CSS_PROP_PADDING(
    padding-right-ltr-source,
    padding_right_ltr_source,
    PaddingRightLTRSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_PADDING(
    padding-right-rtl-source,
    padding_right_rtl_source,
    PaddingRightRTLSource,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        CSS_PROPERTY_DIRECTIONAL_SOURCE,
    "",
    0,
    kBoxPropSourceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_SHORTHAND(
    -moz-padding-start,
    padding_start,
    CSS_PROP_DOMPROP_PREFIXED(PaddingStart),
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_PADDING(
    padding-start-value,
    padding_start_value,
    PaddingStartValue,
    CSS_PROPERTY_PARSE_INACCESSIBLE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_STORES_CALC,
    "",
    VARIANT_HLP | VARIANT_CALC, 
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif
CSS_PROP_PADDING(
    padding-top,
    padding_top,
    PaddingTop,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER |
        
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_HLP | VARIANT_CALC,
    nullptr,
    offsetof(nsStylePadding, mPadding),
    eStyleAnimType_Sides_Top)
CSS_PROP_BACKENDONLY(
    page,
    page,
    Page,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_AUTO | VARIANT_IDENTIFIER,
    nullptr)
CSS_PROP_DISPLAY(
    page-break-after,
    page_break_after,
    PageBreakAfter,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kPageBreakKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None) 
CSS_PROP_DISPLAY(
    page-break-before,
    page_break_before,
    PageBreakBefore,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kPageBreakKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None) 
CSS_PROP_DISPLAY(
    page-break-inside,
    page_break_inside,
    PageBreakInside,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kPageBreakInsideKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SVG(
    paint-order,
    paint_order,
    PaintOrder,
    CSS_PROPERTY_PARSE_FUNCTION,
    "svg.paint-order.enabled",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_VISIBILITY(
    pointer-events,
    pointer_events,
    PointerEvents,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    VARIANT_HK,
    kPointerEventsKTable,
    offsetof(nsStyleVisibility, mPointerEvents),
    eStyleAnimType_EnumU8)
CSS_PROP_DISPLAY(
    position,
    position,
    Position,
    CSS_PROPERTY_PARSE_VALUE |
        
        CSS_PROPERTY_CREATES_STACKING_CONTEXT,
    "",
    VARIANT_HK,
    kPositionKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_QUOTES(
    quotes,
    quotes,
    Quotes,
    CSS_PROPERTY_PARSE_FUNCTION,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_DISPLAY(
    resize,
    resize,
    Resize,
    CSS_PROPERTY_PARSE_VALUE |
        
        
        
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    VARIANT_HK,
    kResizeKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_POSITION(
    right,
    right,
    Right,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_AHLP | VARIANT_CALC,
    nullptr,
    offsetof(nsStylePosition, mOffset),
    eStyleAnimType_Sides_Right)
CSS_PROP_BACKENDONLY(
    size,
    size,
    Size,
    CSS_PROPERTY_PARSE_FUNCTION,
    "",
    0,
    kPageSizeKTable)
CSS_PROP_TABLE(
    table-layout,
    table_layout,
    TableLayout,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kTableLayoutKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_TEXT(
    text-align,
    text_align,
    TextAlign,
    CSS_PROPERTY_PARSE_VALUE | CSS_PROPERTY_VALUE_PARSER_FUNCTION |
      CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    
    
    VARIANT_HK ,
    kTextAlignKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_TEXT(
    -moz-text-align-last,
    text_align_last,
    CSS_PROP_DOMPROP_PREFIXED(TextAlignLast),
    CSS_PROPERTY_PARSE_VALUE | CSS_PROPERTY_VALUE_PARSER_FUNCTION,
    "",
    VARIANT_HK,
    kTextAlignLastKTable,
    offsetof(nsStyleText, mTextAlignLast),
    eStyleAnimType_None)
CSS_PROP_SHORTHAND(
    text-decoration,
    text_decoration,
    TextDecoration,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_TEXT(
    text-combine-upright,
    text_combine_upright,
    TextCombineUpright,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_PARSER_FUNCTION,
    "layout.css.vertical-text.enabled",
    0,
    kTextCombineUprightKTable,
    offsetof(nsStyleText, mTextCombineUpright),
    eStyleAnimType_EnumU8)
CSS_PROP_TEXTRESET(
    -moz-text-decoration-color,
    text_decoration_color,
    CSS_PROP_DOMPROP_PREFIXED(TextDecorationColor),
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED,
    "",
    VARIANT_HCK,
    kBorderColorKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_TEXTRESET(
    -moz-text-decoration-line,
    text_decoration_line,
    CSS_PROP_DOMPROP_PREFIXED(TextDecorationLine),
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_PARSER_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    0,
    kTextDecorationLineKTable,
    offsetof(nsStyleTextReset, mTextDecorationLine),
    eStyleAnimType_EnumU8)
CSS_PROP_TEXTRESET(
    -moz-text-decoration-style,
    text_decoration_style,
    CSS_PROP_DOMPROP_PREFIXED(TextDecorationStyle),
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    VARIANT_HK,
    kTextDecorationStyleKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_TEXT(
    text-indent,
    text_indent,
    TextIndent,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_HLP | VARIANT_CALC,
    nullptr,
    offsetof(nsStyleText, mTextIndent),
    eStyleAnimType_Coord)
CSS_PROP_TEXT(
    text-orientation,
    text_orientation,
    TextOrientation,
    CSS_PROPERTY_PARSE_VALUE,
    "layout.css.vertical-text.enabled",
    VARIANT_HK,
    kTextOrientationKTable,
    offsetof(nsStyleText, mTextOrientation),
    eStyleAnimType_EnumU8)
CSS_PROP_TEXTRESET(
    text-overflow,
    text_overflow,
    TextOverflow,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_PARSER_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    0,
    kTextOverflowKTable,
    offsetof(nsStyleTextReset, mTextOverflow),
    eStyleAnimType_None)
CSS_PROP_TEXT(
    text-shadow,
    text_shadow,
    TextShadow,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS |
        CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED,
        
    "",
    0,
    nullptr,
    offsetof(nsStyleText, mTextShadow),
    eStyleAnimType_Shadow)
CSS_PROP_TEXT(
    -moz-text-size-adjust,
    text_size_adjust,
    CSS_PROP_DOMPROP_PREFIXED(TextSizeAdjust),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_AUTO | VARIANT_NONE | VARIANT_INHERIT,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_TEXT(
    text-transform,
    text_transform,
    TextTransform,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    VARIANT_HK,
    kTextTransformKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_DISPLAY(
    transform,
    transform,
    Transform,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH |
        CSS_PROPERTY_CREATES_STACKING_CONTEXT,
    "",
    0,
    nullptr,
    offsetof(nsStyleDisplay, mSpecifiedTransform),
    eStyleAnimType_Custom)
CSS_PROP_DISPLAY(
    transform-origin,
    transform_origin,
    TransformOrigin,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    0,
    kBackgroundPositionKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_DISPLAY(
    perspective-origin,
    perspective_origin,
    PerspectiveOrigin,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    0,
    kBackgroundPositionKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_DISPLAY(
    perspective,
    perspective,
    Perspective,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_CREATES_STACKING_CONTEXT,
    "",
    VARIANT_NONE | VARIANT_INHERIT | VARIANT_LENGTH | VARIANT_POSITIVE_DIMENSION,
    nullptr,
    offsetof(nsStyleDisplay, mChildPerspective),
    eStyleAnimType_Coord)
CSS_PROP_DISPLAY(
    transform-style,
    transform_style,
    TransformStyle,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_CREATES_STACKING_CONTEXT,
    "",
    VARIANT_HK,
    kTransformStyleKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_DISPLAY(
    backface-visibility,
    backface_visibility,
    BackfaceVisibility,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kBackfaceVisibilityKTable,
    offsetof(nsStyleDisplay, mBackfaceVisibility),
    eStyleAnimType_None)
CSS_PROP_POSITION(
    top,
    top,
    Top,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_AHLP | VARIANT_CALC,
    nullptr,
    offsetof(nsStylePosition, mOffset),
    eStyleAnimType_Sides_Top)
 CSS_PROP_DISPLAY(
    touch-action,
    touch_action,
    TouchAction,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_PARSER_FUNCTION,
    "layout.css.touch_action.enabled",
    VARIANT_HK,
    kTouchActionKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SHORTHAND(
    transition,
    transition,
    Transition,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_DISPLAY(
    transition-delay,
    transition_delay,
    TransitionDelay,
    CSS_PROPERTY_PARSE_VALUE_LIST |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS,
    "",
    VARIANT_TIME, 
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_DISPLAY(
    transition-duration,
    transition_duration,
    TransitionDuration,
    CSS_PROPERTY_PARSE_VALUE_LIST |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS,
    "",
    VARIANT_TIME | VARIANT_NONNEGATIVE_DIMENSION, 
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_DISPLAY(
    transition-property,
    transition_property,
    TransitionProperty,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS,
    "",
    VARIANT_IDENTIFIER | VARIANT_NONE | VARIANT_ALL, 
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_DISPLAY(
    transition-timing-function,
    transition_timing_function,
    TransitionTimingFunction,
    CSS_PROPERTY_PARSE_VALUE_LIST |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS,
    "",
    VARIANT_KEYWORD | VARIANT_TIMING_FUNCTION, 
    kTransitionTimingFunctionKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#ifndef CSS_PROP_LIST_ONLY_COMPONENTS_OF_ALL_SHORTHAND
CSS_PROP_TEXTRESET(
    unicode-bidi,
    unicode_bidi,
    UnicodeBidi,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kUnicodeBidiKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif 
CSS_PROP_USERINTERFACE(
    -moz-user-focus,
    user_focus,
    CSS_PROP_DOMPROP_PREFIXED(UserFocus),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kUserFocusKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None) 
CSS_PROP_USERINTERFACE(
    -moz-user-input,
    user_input,
    CSS_PROP_DOMPROP_PREFIXED(UserInput),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kUserInputKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None) 
CSS_PROP_USERINTERFACE(
    -moz-user-modify,
    user_modify,
    CSS_PROP_DOMPROP_PREFIXED(UserModify),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kUserModifyKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None) 
CSS_PROP_UIRESET(
    -moz-user-select,
    user_select,
    CSS_PROP_DOMPROP_PREFIXED(UserSelect),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kUserSelectKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None) 



CSS_PROP_TEXTRESET(
    vertical-align,
    vertical_align,
    VerticalAlign,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_HKLP | VARIANT_CALC,
    kVerticalAlignKTable,
    offsetof(nsStyleTextReset, mVerticalAlign),
    eStyleAnimType_Coord)
CSS_PROP_VISIBILITY(
    visibility,
    visibility,
    Visibility,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kVisibilityKTable,
    offsetof(nsStyleVisibility, mVisible),
    eStyleAnimType_EnumU8)  
CSS_PROP_TEXT(
    white-space,
    white_space,
    WhiteSpace,
    CSS_PROPERTY_PARSE_VALUE |
        
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER,
    "",
    VARIANT_HK,
    kWhitespaceKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_BACKENDONLY(
    widows,
    widows,
    Widows,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_AT_LEAST_ONE,
    "",
    VARIANT_HI,
    nullptr)
CSS_PROP_POSITION(
    width,
    width,
    Width,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_STORES_CALC |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK |
        CSS_PROPERTY_GETCS_NEEDS_LAYOUT_FLUSH,
    "",
    VARIANT_AHKLP | VARIANT_CALC,
    kWidthKTable,
    offsetof(nsStylePosition, mWidth),
    eStyleAnimType_Coord)
CSS_PROP_UIRESET(
    -moz-window-shadow,
    _moz_window_shadow,
    CSS_PROP_DOMPROP_PREFIXED(WindowShadow),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kWindowShadowKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_TEXT(
    word-break,
    word_break,
    WordBreak,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kWordBreakKTable,
    offsetof(nsStyleText, mWordBreak),
    eStyleAnimType_EnumU8)
CSS_PROP_TEXT(
    word-spacing,
    word_spacing,
    WordSpacing,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_APPLIES_TO_FIRST_LETTER_AND_FIRST_LINE |
        CSS_PROPERTY_APPLIES_TO_PLACEHOLDER |
        CSS_PROPERTY_UNITLESS_LENGTH_QUIRK,
    "",
    VARIANT_HL | VARIANT_NORMAL | VARIANT_CALC,
    nullptr,
    offsetof(nsStyleText, mWordSpacing),
    eStyleAnimType_nscoord)
CSS_PROP_TEXT(
    word-wrap,
    word_wrap,
    WordWrap,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kWordWrapKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_TEXT(
    -moz-hyphens,
    hyphens,
    CSS_PROP_DOMPROP_PREFIXED(Hyphens),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kHyphensKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_VISIBILITY(
    writing-mode,
    writing_mode,
    WritingMode,
    CSS_PROPERTY_PARSE_VALUE,
    "layout.css.vertical-text.enabled",
    VARIANT_HK,
    kWritingModeKTable,
    offsetof(nsStyleVisibility, mWritingMode),
    eStyleAnimType_EnumU8)
CSS_PROP_POSITION(
    z-index,
    z_index,
    ZIndex,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_CREATES_STACKING_CONTEXT,
    "",
    VARIANT_AHI,
    nullptr,
    offsetof(nsStylePosition, mZIndex),
    eStyleAnimType_Coord)
CSS_PROP_XUL(
    -moz-box-align,
    box_align,
    CSS_PROP_DOMPROP_PREFIXED(BoxAlign),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kBoxAlignKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None) 
CSS_PROP_XUL(
    -moz-box-direction,
    box_direction,
    CSS_PROP_DOMPROP_PREFIXED(BoxDirection),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kBoxDirectionKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None) 
CSS_PROP_XUL(
    -moz-box-flex,
    box_flex,
    CSS_PROP_DOMPROP_PREFIXED(BoxFlex),
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE,
    "",
    VARIANT_HN,
    nullptr,
    offsetof(nsStyleXUL, mBoxFlex),
    eStyleAnimType_float) 
CSS_PROP_XUL(
    -moz-box-orient,
    box_orient,
    CSS_PROP_DOMPROP_PREFIXED(BoxOrient),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kBoxOrientKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None) 
CSS_PROP_XUL(
    -moz-box-pack,
    box_pack,
    CSS_PROP_DOMPROP_PREFIXED(BoxPack),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kBoxPackKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None) 
CSS_PROP_XUL(
    -moz-box-ordinal-group,
    box_ordinal_group,
    CSS_PROP_DOMPROP_PREFIXED(BoxOrdinalGroup),
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE,
    "",
    VARIANT_HI,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_XUL(
    -moz-stack-sizing,
    stack_sizing,
    CSS_PROP_DOMPROP_PREFIXED(StackSizing),
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kStackSizingKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)

#ifndef CSS_PROP_LIST_ONLY_COMPONENTS_OF_ALL_SHORTHAND
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL
CSS_PROP_FONT(
    -moz-script-level,
    script_level,
    ScriptLevel,
    
    
    
    CSS_PROPERTY_PARSE_VALUE,
    "",
    
    
    
    VARIANT_AHI,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_FONT(
    -moz-script-size-multiplier,
    script_size_multiplier,
    ScriptSizeMultiplier,
    
    CSS_PROPERTY_PARSE_INACCESSIBLE,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_FONT(
    -moz-script-min-size,
    script_min_size,
    ScriptMinSize,
    
    CSS_PROPERTY_PARSE_INACCESSIBLE,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_FONT(
    -moz-math-variant,
    math_variant,
    MathVariant,
    CSS_PROPERTY_PARSE_INACCESSIBLE,
    "",
    VARIANT_HK,
    kMathVariantKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_FONT(
    -moz-math-display,
    math_display,
    MathDisplay,
    
    
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kMathDisplayKTable,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif 
#endif 

CSS_PROP_SVGRESET(
    clip-path,
    clip_path,
    ClipPath,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_CREATES_STACKING_CONTEXT,
    "",
    VARIANT_HUO,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SVG(
    clip-rule,
    clip_rule,
    ClipRule,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kFillRuleKTable,
    offsetof(nsStyleSVG, mClipRule),
    eStyleAnimType_EnumU8)
CSS_PROP_SVG(
    color-interpolation,
    color_interpolation,
    ColorInterpolation,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kColorInterpolationKTable,
    offsetof(nsStyleSVG, mColorInterpolation),
    eStyleAnimType_EnumU8)
CSS_PROP_SVG(
    color-interpolation-filters,
    color_interpolation_filters,
    ColorInterpolationFilters,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kColorInterpolationKTable,
    offsetof(nsStyleSVG, mColorInterpolationFilters),
    eStyleAnimType_EnumU8)
CSS_PROP_SVGRESET(
    dominant-baseline,
    dominant_baseline,
    DominantBaseline,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kDominantBaselineKTable,
    offsetof(nsStyleSVGReset, mDominantBaseline),
    eStyleAnimType_EnumU8)
CSS_PROP_SVG(
    fill,
    fill,
    Fill,
    CSS_PROPERTY_PARSE_FUNCTION,
    "",
    0,
    kContextPatternKTable,
    offsetof(nsStyleSVG, mFill),
    eStyleAnimType_PaintServer)
CSS_PROP_SVG(
    fill-opacity,
    fill_opacity,
    FillOpacity,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HN | VARIANT_OPENTYPE_SVG_KEYWORD,
    kContextOpacityKTable,
    offsetof(nsStyleSVG, mFillOpacity),
    eStyleAnimType_float)
CSS_PROP_SVG(
    fill-rule,
    fill_rule,
    FillRule,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kFillRuleKTable,
    offsetof(nsStyleSVG, mFillRule),
    eStyleAnimType_EnumU8)
CSS_PROP_SVGRESET(
    filter,
    filter,
    Filter,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_CREATES_STACKING_CONTEXT,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_Custom)
CSS_PROP_SVGRESET(
    flood-color,
    flood_color,
    FloodColor,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HC,
    nullptr,
    offsetof(nsStyleSVGReset, mFloodColor),
    eStyleAnimType_Color)
CSS_PROP_SVGRESET(
    flood-opacity,
    flood_opacity,
    FloodOpacity,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HN,
    nullptr,
    offsetof(nsStyleSVGReset, mFloodOpacity),
    eStyleAnimType_float)
CSS_PROP_SVG(
    image-rendering,
    image_rendering,
    ImageRendering,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kImageRenderingKTable,
    offsetof(nsStyleSVG, mImageRendering),
    eStyleAnimType_EnumU8)
CSS_PROP_SVGRESET(
    lighting-color,
    lighting_color,
    LightingColor,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HC,
    nullptr,
    offsetof(nsStyleSVGReset, mLightingColor),
    eStyleAnimType_Color)
CSS_PROP_SHORTHAND(
    marker,
    marker,
    Marker,
    CSS_PROPERTY_PARSE_FUNCTION,
    "")
CSS_PROP_SVG(
    marker-end,
    marker_end,
    MarkerEnd,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HUO,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SVG(
    marker-mid,
    marker_mid,
    MarkerMid,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HUO,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SVG(
    marker-start,
    marker_start,
    MarkerStart,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HUO,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SVGRESET(
    mask,
    mask,
    Mask,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_CREATES_STACKING_CONTEXT,
    "",
    VARIANT_HUO,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_SVGRESET(
    mask-type,
    mask_type,
    MaskType,
    CSS_PROPERTY_PARSE_VALUE,
    "layout.css.masking.enabled",
    VARIANT_HK,
    kMaskTypeKTable,
    offsetof(nsStyleSVGReset, mMaskType),
    eStyleAnimType_EnumU8)
CSS_PROP_SVG(
    shape-rendering,
    shape_rendering,
    ShapeRendering,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kShapeRenderingKTable,
    offsetof(nsStyleSVG, mShapeRendering),
    eStyleAnimType_EnumU8)
CSS_PROP_SVGRESET(
    stop-color,
    stop_color,
    StopColor,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HC,
    nullptr,
    offsetof(nsStyleSVGReset, mStopColor),
    eStyleAnimType_Color)
CSS_PROP_SVGRESET(
    stop-opacity,
    stop_opacity,
    StopOpacity,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HN,
    nullptr,
    offsetof(nsStyleSVGReset, mStopOpacity),
    eStyleAnimType_float)
CSS_PROP_SVG(
    stroke,
    stroke,
    Stroke,
    CSS_PROPERTY_PARSE_FUNCTION,
    "",
    0,
    kContextPatternKTable,
    offsetof(nsStyleSVG, mStroke),
    eStyleAnimType_PaintServer)
CSS_PROP_SVG(
    stroke-dasharray,
    stroke_dasharray,
    StrokeDasharray,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS |
        CSS_PROPERTY_NUMBERS_ARE_PIXELS,
        
    "",
    0,
    kStrokeContextValueKTable,
    CSS_PROP_NO_OFFSET, 
    eStyleAnimType_Custom)
CSS_PROP_SVG(
    stroke-dashoffset,
    stroke_dashoffset,
    StrokeDashoffset,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_NUMBERS_ARE_PIXELS,
    "",
    VARIANT_HLPN | VARIANT_OPENTYPE_SVG_KEYWORD,
    kStrokeContextValueKTable,
    offsetof(nsStyleSVG, mStrokeDashoffset),
    eStyleAnimType_Coord)
CSS_PROP_SVG(
    stroke-linecap,
    stroke_linecap,
    StrokeLinecap,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kStrokeLinecapKTable,
    offsetof(nsStyleSVG, mStrokeLinecap),
    eStyleAnimType_EnumU8)
CSS_PROP_SVG(
    stroke-linejoin,
    stroke_linejoin,
    StrokeLinejoin,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kStrokeLinejoinKTable,
    offsetof(nsStyleSVG, mStrokeLinejoin),
    eStyleAnimType_EnumU8)
CSS_PROP_SVG(
    stroke-miterlimit,
    stroke_miterlimit,
    StrokeMiterlimit,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_AT_LEAST_ONE,
    "",
    VARIANT_HN,
    nullptr,
    offsetof(nsStyleSVG, mStrokeMiterlimit),
    eStyleAnimType_float)
CSS_PROP_SVG(
    stroke-opacity,
    stroke_opacity,
    StrokeOpacity,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HN | VARIANT_OPENTYPE_SVG_KEYWORD,
    kContextOpacityKTable,
    offsetof(nsStyleSVG, mStrokeOpacity),
    eStyleAnimType_float)
CSS_PROP_SVG(
    stroke-width,
    stroke_width,
    StrokeWidth,
    CSS_PROPERTY_PARSE_VALUE |
        CSS_PROPERTY_VALUE_NONNEGATIVE |
        CSS_PROPERTY_NUMBERS_ARE_PIXELS,
    "",
    VARIANT_HLPN | VARIANT_OPENTYPE_SVG_KEYWORD,
    kStrokeContextValueKTable,
    offsetof(nsStyleSVG, mStrokeWidth),
    eStyleAnimType_Coord)
CSS_PROP_SVG(
    text-anchor,
    text_anchor,
    TextAnchor,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kTextAnchorKTable,
    offsetof(nsStyleSVG, mTextAnchor),
    eStyleAnimType_EnumU8)
CSS_PROP_SVG(
    text-rendering,
    text_rendering,
    TextRendering,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kTextRenderingKTable,
    offsetof(nsStyleSVG, mTextRendering),
    eStyleAnimType_EnumU8)
CSS_PROP_SVGRESET(
    vector-effect,
    vector_effect,
    VectorEffect,
    CSS_PROPERTY_PARSE_VALUE,
    "",
    VARIANT_HK,
    kVectorEffectKTable,
    offsetof(nsStyleSVGReset, mVectorEffect),
    eStyleAnimType_EnumU8)

CSS_PROP_DISPLAY(
    will-change,
    will_change,
    WillChange,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_VALUE_LIST_USES_COMMAS |
        CSS_PROPERTY_ALWAYS_ENABLED_IN_CHROME_OR_CERTIFIED_APP,
    "layout.css.will-change.enabled",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)



CSS_PROP_SHORTHAND(
    -moz-transform,
    _moz_transform,
    MozTransform,
    CSS_PROPERTY_PARSE_FUNCTION |
        CSS_PROPERTY_IS_ALIAS,
    "layout.css.prefixes.transforms")

#ifndef CSS_PROP_LIST_ONLY_COMPONENTS_OF_ALL_SHORTHAND
#ifndef CSS_PROP_LIST_EXCLUDE_INTERNAL






#ifdef CSS_PROP_STUB_NOT_CSS
CSS_PROP_STUB_NOT_CSS
CSS_PROP_STUB_NOT_CSS
#else
CSS_PROP_FONT(
    -x-lang,
    _x_lang,
    Lang,
    CSS_PROPERTY_PARSE_INACCESSIBLE,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_TABLE(
    -x-span,
    _x_span,
    Span,
    CSS_PROPERTY_PARSE_INACCESSIBLE,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
CSS_PROP_FONT(
    -x-text-zoom,
    _x_text_zoom,
    TextZoom,
    CSS_PROPERTY_PARSE_INACCESSIBLE,
    "",
    0,
    nullptr,
    CSS_PROP_NO_OFFSET,
    eStyleAnimType_None)
#endif 
#endif 
#endif 

#ifdef USED_CSS_PROP

#undef USED_CSS_PROP
#undef CSS_PROP_FONT
#undef CSS_PROP_COLOR
#undef CSS_PROP_BACKGROUND
#undef CSS_PROP_LIST
#undef CSS_PROP_POSITION
#undef CSS_PROP_TEXT
#undef CSS_PROP_TEXTRESET
#undef CSS_PROP_DISPLAY
#undef CSS_PROP_VISIBILITY
#undef CSS_PROP_CONTENT
#undef CSS_PROP_QUOTES
#undef CSS_PROP_USERINTERFACE
#undef CSS_PROP_UIRESET
#undef CSS_PROP_TABLE
#undef CSS_PROP_TABLEBORDER
#undef CSS_PROP_MARGIN
#undef CSS_PROP_PADDING
#undef CSS_PROP_BORDER
#undef CSS_PROP_OUTLINE
#undef CSS_PROP_XUL
#undef CSS_PROP_COLUMN
#undef CSS_PROP_SVG
#undef CSS_PROP_SVGRESET
#undef CSS_PROP_VARIABLES
#ifdef DEFINED_CSS_PROP_BACKENDONLY
#undef CSS_PROP_BACKENDONLY
#undef DEFINED_CSS_PROP_BACKENDONLY
#endif

#else 

#ifdef DEFINED_CSS_PROP_FONT
#undef CSS_PROP_FONT
#undef DEFINED_CSS_PROP_FONT
#endif
#ifdef DEFINED_CSS_PROP_COLOR
#undef CSS_PROP_COLOR
#undef DEFINED_CSS_PROP_COLOR
#endif
#ifdef DEFINED_CSS_PROP_BACKGROUND
#undef CSS_PROP_BACKGROUND
#undef DEFINED_CSS_PROP_BACKGROUND
#endif
#ifdef DEFINED_CSS_PROP_LIST
#undef CSS_PROP_LIST
#undef DEFINED_CSS_PROP_LIST
#endif
#ifdef DEFINED_CSS_PROP_POSITION
#undef CSS_PROP_POSITION
#undef DEFINED_CSS_PROP_POSITION
#endif
#ifdef DEFINED_CSS_PROP_TEXT
#undef CSS_PROP_TEXT
#undef DEFINED_CSS_PROP_TETEXTRESETT
#endif
#ifdef DEFINED_CSS_PROP_TEXTRESET
#undef CSS_PROP_TEXTRESET
#undef DEFINED_CSS_PROP_TEDISPLAYTRESET
#endif
#ifdef DEFINED_CSS_PROP_DISPLAY
#undef CSS_PROP_DISPLAY
#undef DEFINED_CSS_PROP_DISPLAY
#endif
#ifdef DEFINED_CSS_PROP_VISIBILITY
#undef CSS_PROP_VISIBILITY
#undef DEFINED_CSS_PROP_VISIBILITY
#endif
#ifdef DEFINED_CSS_PROP_CONTENT
#undef CSS_PROP_CONTENT
#undef DEFINED_CSS_PROP_CONTENT
#endif
#ifdef DEFINED_CSS_PROP_QUOTES
#undef CSS_PROP_QUOTES
#undef DEFINED_CSS_PROP_QUOTES
#endif
#ifdef DEFINED_CSS_PROP_USERINTERFACE
#undef CSS_PROP_USERINTERFACE
#undef DEFINED_CSS_PROP_USERINTERFACE
#endif
#ifdef DEFINED_CSS_PROP_UIRESET
#undef CSS_PROP_UIRESET
#undef DEFINED_CSS_PROP_UIRESET
#endif
#ifdef DEFINED_CSS_PROP_TABLE
#undef CSS_PROP_TABLE
#undef DEFINED_CSS_PROP_TABLE
#endif
#ifdef DEFINED_CSS_PROP_TABLEBORDER
#undef CSS_PROP_TABLEBORDER
#undef DEFINED_CSS_PROP_TABLEBORDER
#endif
#ifdef DEFINED_CSS_PROP_MARGIN
#undef CSS_PROP_MARGIN
#undef DEFINED_CSS_PROP_MARGIN
#endif
#ifdef DEFINED_CSS_PROP_PADDING
#undef CSS_PROP_PADDING
#undef DEFINED_CSS_PROP_PADDING
#endif
#ifdef DEFINED_CSS_PROP_BORDER
#undef CSS_PROP_BORDER
#undef DEFINED_CSS_PROP_BORDER
#endif
#ifdef DEFINED_CSS_PROP_OUTLINE
#undef CSS_PROP_OUTLINE
#undef DEFINED_CSS_PROP_OUTLINE
#endif
#ifdef DEFINED_CSS_PROP_XUL
#undef CSS_PROP_XUL
#undef DEFINED_CSS_PROP_XUL
#endif
#ifdef DEFINED_CSS_PROP_COLUMN
#undef CSS_PROP_COLUMN
#undef DEFINED_CSS_PROP_COLUMN
#endif
#ifdef DEFINED_CSS_PROP_SVG
#undef CSS_PROP_SVG
#undef DEFINED_CSS_PROP_SVG
#endif
#ifdef DEFINED_CSS_PROP_SVGRESET
#undef CSS_PROP_SVGRESET
#undef DEFINED_CSS_PROP_SVGRESET
#endif
#ifdef DEFINED_CSS_PROP_VARIABLES
#undef CSS_PROP_VARIABLES
#undef DEFINED_CSS_PROP_VARIABLES
#endif
#ifdef DEFINED_CSS_PROP_BACKENDONLY
#undef CSS_PROP_BACKENDONLY
#undef DEFINED_CSS_PROP_BACKENDONLY
#endif

#endif 

#ifdef DEFINED_CSS_PROP_SHORTHAND
#undef CSS_PROP_SHORTHAND
#undef DEFINED_CSS_PROP_SHORTHAND
#endif

#undef CSS_PROP_DOMPROP_PREFIXED
