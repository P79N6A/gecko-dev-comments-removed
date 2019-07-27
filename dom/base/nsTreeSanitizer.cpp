





#include "nsTreeSanitizer.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/CSSStyleSheet.h"
#include "mozilla/css/Declaration.h"
#include "mozilla/css/StyleRule.h"
#include "mozilla/css/Rule.h"
#include "nsCSSParser.h"
#include "nsCSSProperty.h"
#include "nsUnicharInputStream.h"
#include "nsIDOMCSSRule.h"
#include "nsAttrName.h"
#include "nsIScriptSecurityManager.h"
#include "nsNetUtil.h"
#include "nsComponentManagerUtils.h"
#include "nsNullPrincipal.h"
#include "nsContentUtils.h"
#include "nsIParserUtils.h"
#include "nsIDocument.h"
#include "nsQueryObject.h"

using namespace mozilla;




nsIAtom** const kElementsHTML[] = {
  &nsGkAtoms::a,
  &nsGkAtoms::abbr,
  &nsGkAtoms::acronym,
  &nsGkAtoms::address,
  &nsGkAtoms::area,
  &nsGkAtoms::article,
  &nsGkAtoms::aside,
  &nsGkAtoms::audio,
  &nsGkAtoms::b,
  &nsGkAtoms::bdi,
  &nsGkAtoms::bdo,
  &nsGkAtoms::big,
  &nsGkAtoms::blockquote,
  
  &nsGkAtoms::br,
  &nsGkAtoms::button,
  &nsGkAtoms::canvas,
  &nsGkAtoms::caption,
  &nsGkAtoms::center,
  &nsGkAtoms::cite,
  &nsGkAtoms::code,
  &nsGkAtoms::col,
  &nsGkAtoms::colgroup,
  &nsGkAtoms::datalist,
  &nsGkAtoms::dd,
  &nsGkAtoms::del,
  &nsGkAtoms::details,
  &nsGkAtoms::dfn,
  &nsGkAtoms::dir,
  &nsGkAtoms::div,
  &nsGkAtoms::dl,
  &nsGkAtoms::dt,
  &nsGkAtoms::em,
  &nsGkAtoms::fieldset,
  &nsGkAtoms::figcaption,
  &nsGkAtoms::figure,
  &nsGkAtoms::font,
  &nsGkAtoms::footer,
  &nsGkAtoms::form,
  &nsGkAtoms::h1,
  &nsGkAtoms::h2,
  &nsGkAtoms::h3,
  &nsGkAtoms::h4,
  &nsGkAtoms::h5,
  &nsGkAtoms::h6,
  
  &nsGkAtoms::header,
  &nsGkAtoms::hgroup,
  &nsGkAtoms::hr,
  
  &nsGkAtoms::i,
  &nsGkAtoms::img,
  &nsGkAtoms::input,
  &nsGkAtoms::ins,
  &nsGkAtoms::kbd,
  &nsGkAtoms::label,
  &nsGkAtoms::legend,
  &nsGkAtoms::li,
  &nsGkAtoms::link,
  &nsGkAtoms::listing,
  &nsGkAtoms::map,
  &nsGkAtoms::mark,
  &nsGkAtoms::menu,
  &nsGkAtoms::meta,
  &nsGkAtoms::meter,
  &nsGkAtoms::nav,
  &nsGkAtoms::nobr,
  &nsGkAtoms::noscript,
  &nsGkAtoms::ol,
  &nsGkAtoms::optgroup,
  &nsGkAtoms::option,
  &nsGkAtoms::output,
  &nsGkAtoms::p,
  &nsGkAtoms::pre,
  &nsGkAtoms::progress,
  &nsGkAtoms::q,
  &nsGkAtoms::rb,
  &nsGkAtoms::rp,
  &nsGkAtoms::rt,
  &nsGkAtoms::rtc,
  &nsGkAtoms::ruby,
  &nsGkAtoms::s,
  &nsGkAtoms::samp,
  &nsGkAtoms::section,
  &nsGkAtoms::select,
  &nsGkAtoms::small,
  &nsGkAtoms::source,
  &nsGkAtoms::span,
  &nsGkAtoms::strike,
  &nsGkAtoms::strong,
  &nsGkAtoms::sub,
  &nsGkAtoms::summary,
  &nsGkAtoms::sup,
  
  &nsGkAtoms::table,
  &nsGkAtoms::tbody,
  &nsGkAtoms::td,
  &nsGkAtoms::textarea,
  &nsGkAtoms::tfoot,
  &nsGkAtoms::th,
  &nsGkAtoms::thead,
  &nsGkAtoms::time,
  
  &nsGkAtoms::tr,
  &nsGkAtoms::track,
  &nsGkAtoms::tt,
  &nsGkAtoms::u,
  &nsGkAtoms::ul,
  &nsGkAtoms::var,
  &nsGkAtoms::video,
  &nsGkAtoms::wbr,
  nullptr
};

nsIAtom** const kAttributesHTML[] = {
  &nsGkAtoms::abbr,
  &nsGkAtoms::accept,
  &nsGkAtoms::acceptcharset,
  &nsGkAtoms::accesskey,
  &nsGkAtoms::action,
  &nsGkAtoms::alt,
  &nsGkAtoms::autocomplete,
  &nsGkAtoms::autofocus,
  &nsGkAtoms::autoplay,
  &nsGkAtoms::axis,
  &nsGkAtoms::_char,
  &nsGkAtoms::charoff,
  &nsGkAtoms::charset,
  &nsGkAtoms::checked,
  &nsGkAtoms::cite,
  &nsGkAtoms::_class,
  &nsGkAtoms::cols,
  &nsGkAtoms::colspan,
  &nsGkAtoms::content,
  &nsGkAtoms::contenteditable,
  &nsGkAtoms::contextmenu,
  &nsGkAtoms::controls,
  &nsGkAtoms::coords,
  &nsGkAtoms::datetime,
  &nsGkAtoms::dir,
  &nsGkAtoms::disabled,
  &nsGkAtoms::draggable,
  &nsGkAtoms::enctype,
  &nsGkAtoms::face,
  &nsGkAtoms::_for,
  &nsGkAtoms::frame,
  &nsGkAtoms::headers,
  &nsGkAtoms::height,
  &nsGkAtoms::hidden,
  &nsGkAtoms::high,
  &nsGkAtoms::href,
  &nsGkAtoms::hreflang,
  &nsGkAtoms::icon,
  &nsGkAtoms::id,
  &nsGkAtoms::ismap,
  &nsGkAtoms::itemid,
  &nsGkAtoms::itemprop,
  &nsGkAtoms::itemref,
  &nsGkAtoms::itemscope,
  &nsGkAtoms::itemtype,
  &nsGkAtoms::kind,
  &nsGkAtoms::label,
  &nsGkAtoms::lang,
  &nsGkAtoms::list,
  &nsGkAtoms::longdesc,
  &nsGkAtoms::loop,
  &nsGkAtoms::low,
  &nsGkAtoms::max,
  &nsGkAtoms::maxlength,
  &nsGkAtoms::media,
  &nsGkAtoms::method,
  &nsGkAtoms::min,
  &nsGkAtoms::mozdonotsend,
  &nsGkAtoms::multiple,
  &nsGkAtoms::muted,
  &nsGkAtoms::name,
  &nsGkAtoms::nohref,
  &nsGkAtoms::novalidate,
  &nsGkAtoms::nowrap,
  &nsGkAtoms::open,
  &nsGkAtoms::optimum,
  &nsGkAtoms::pattern,
  &nsGkAtoms::placeholder,
  &nsGkAtoms::playbackrate,
  &nsGkAtoms::poster,
  &nsGkAtoms::preload,
  &nsGkAtoms::prompt,
  &nsGkAtoms::pubdate,
  &nsGkAtoms::radiogroup,
  &nsGkAtoms::readonly,
  &nsGkAtoms::rel,
  &nsGkAtoms::required,
  &nsGkAtoms::rev,
  &nsGkAtoms::reversed,
  &nsGkAtoms::role,
  &nsGkAtoms::rows,
  &nsGkAtoms::rowspan,
  &nsGkAtoms::rules,
  &nsGkAtoms::scoped,
  &nsGkAtoms::scope,
  &nsGkAtoms::selected,
  &nsGkAtoms::shape,
  &nsGkAtoms::span,
  &nsGkAtoms::spellcheck,
  &nsGkAtoms::src,
  &nsGkAtoms::srclang,
  &nsGkAtoms::start,
  &nsGkAtoms::summary,
  &nsGkAtoms::tabindex,
  &nsGkAtoms::target,
  &nsGkAtoms::title,
  &nsGkAtoms::type,
  &nsGkAtoms::usemap,
  &nsGkAtoms::value,
  &nsGkAtoms::width,
  &nsGkAtoms::wrap,
  nullptr
};

nsIAtom** const kPresAttributesHTML[] = {
  &nsGkAtoms::align,
  &nsGkAtoms::background,
  &nsGkAtoms::bgcolor,
  &nsGkAtoms::border,
  &nsGkAtoms::cellpadding,
  &nsGkAtoms::cellspacing,
  &nsGkAtoms::color,
  &nsGkAtoms::compact,
  &nsGkAtoms::clear,
  &nsGkAtoms::hspace,
  &nsGkAtoms::noshade,
  &nsGkAtoms::pointSize,
  &nsGkAtoms::size,
  &nsGkAtoms::valign,
  &nsGkAtoms::vspace,
  nullptr
};

nsIAtom** const kURLAttributesHTML[] = {
  &nsGkAtoms::action,
  &nsGkAtoms::href,
  &nsGkAtoms::src,
  &nsGkAtoms::longdesc,
  &nsGkAtoms::cite,
  &nsGkAtoms::background,
  nullptr
};

nsIAtom** const kElementsSVG[] = {
  &nsGkAtoms::a, 
  &nsGkAtoms::altGlyph, 
  &nsGkAtoms::altGlyphDef, 
  &nsGkAtoms::altGlyphItem, 
  &nsGkAtoms::animate, 
  &nsGkAtoms::animateColor, 
  &nsGkAtoms::animateMotion, 
  &nsGkAtoms::animateTransform, 
  &nsGkAtoms::circle, 
  &nsGkAtoms::clipPath, 
  &nsGkAtoms::colorProfile, 
  &nsGkAtoms::cursor, 
  &nsGkAtoms::defs, 
  &nsGkAtoms::desc, 
  &nsGkAtoms::ellipse, 
  &nsGkAtoms::elevation, 
  &nsGkAtoms::erode, 
  &nsGkAtoms::ex, 
  &nsGkAtoms::exact, 
  &nsGkAtoms::exponent, 
  &nsGkAtoms::feBlend, 
  &nsGkAtoms::feColorMatrix, 
  &nsGkAtoms::feComponentTransfer, 
  &nsGkAtoms::feComposite, 
  &nsGkAtoms::feConvolveMatrix, 
  &nsGkAtoms::feDiffuseLighting, 
  &nsGkAtoms::feDisplacementMap, 
  &nsGkAtoms::feDistantLight, 
  &nsGkAtoms::feDropShadow, 
  &nsGkAtoms::feFlood, 
  &nsGkAtoms::feFuncA, 
  &nsGkAtoms::feFuncB, 
  &nsGkAtoms::feFuncG, 
  &nsGkAtoms::feFuncR, 
  &nsGkAtoms::feGaussianBlur, 
  &nsGkAtoms::feImage, 
  &nsGkAtoms::feMerge, 
  &nsGkAtoms::feMergeNode, 
  &nsGkAtoms::feMorphology, 
  &nsGkAtoms::feOffset, 
  &nsGkAtoms::fePointLight, 
  &nsGkAtoms::feSpecularLighting, 
  &nsGkAtoms::feSpotLight, 
  &nsGkAtoms::feTile, 
  &nsGkAtoms::feTurbulence, 
  &nsGkAtoms::filter, 
  &nsGkAtoms::font, 
  &nsGkAtoms::font_face, 
  &nsGkAtoms::font_face_format, 
  &nsGkAtoms::font_face_name, 
  &nsGkAtoms::font_face_src, 
  &nsGkAtoms::font_face_uri, 
  &nsGkAtoms::foreignObject, 
  &nsGkAtoms::g, 
  &nsGkAtoms::glyph, 
  &nsGkAtoms::glyphRef, 
  &nsGkAtoms::hkern, 
  &nsGkAtoms::image, 
  &nsGkAtoms::line, 
  &nsGkAtoms::linearGradient, 
  &nsGkAtoms::marker, 
  &nsGkAtoms::mask, 
  &nsGkAtoms::metadata, 
  &nsGkAtoms::missingGlyph, 
  &nsGkAtoms::mpath, 
  &nsGkAtoms::path, 
  &nsGkAtoms::pattern, 
  &nsGkAtoms::polygon, 
  &nsGkAtoms::polyline, 
  &nsGkAtoms::radialGradient, 
  &nsGkAtoms::rect, 
  &nsGkAtoms::set, 
  &nsGkAtoms::stop, 
  &nsGkAtoms::svg, 
  &nsGkAtoms::svgSwitch, 
  &nsGkAtoms::symbol, 
  &nsGkAtoms::text, 
  &nsGkAtoms::textPath, 
  &nsGkAtoms::title, 
  &nsGkAtoms::tref, 
  &nsGkAtoms::tspan, 
  &nsGkAtoms::use, 
  &nsGkAtoms::view, 
  &nsGkAtoms::vkern, 
  nullptr
};

nsIAtom** const kAttributesSVG[] = {
  
  &nsGkAtoms::accumulate, 
  &nsGkAtoms::additive, 
  &nsGkAtoms::alignment_baseline, 
  
  &nsGkAtoms::amplitude, 
  
  
  &nsGkAtoms::attributeName, 
  &nsGkAtoms::attributeType, 
  &nsGkAtoms::azimuth, 
  &nsGkAtoms::baseFrequency, 
  &nsGkAtoms::baseline_shift, 
  
  
  &nsGkAtoms::begin, 
  &nsGkAtoms::bias, 
  &nsGkAtoms::by, 
  &nsGkAtoms::calcMode, 
  
  &nsGkAtoms::_class, 
  &nsGkAtoms::clip_path, 
  &nsGkAtoms::clip_rule, 
  &nsGkAtoms::clipPathUnits, 
  &nsGkAtoms::color, 
  &nsGkAtoms::colorInterpolation, 
  &nsGkAtoms::colorInterpolationFilters, 
  &nsGkAtoms::cursor, 
  &nsGkAtoms::cx, 
  &nsGkAtoms::cy, 
  &nsGkAtoms::d, 
  
  &nsGkAtoms::diffuseConstant, 
  &nsGkAtoms::direction, 
  &nsGkAtoms::display, 
  &nsGkAtoms::divisor, 
  &nsGkAtoms::dominant_baseline, 
  &nsGkAtoms::dur, 
  &nsGkAtoms::dx, 
  &nsGkAtoms::dy, 
  &nsGkAtoms::edgeMode, 
  &nsGkAtoms::elevation, 
  
  &nsGkAtoms::end, 
  &nsGkAtoms::fill, 
  &nsGkAtoms::fill_opacity, 
  &nsGkAtoms::fill_rule, 
  &nsGkAtoms::filter, 
  &nsGkAtoms::filterUnits, 
  &nsGkAtoms::flood_color, 
  &nsGkAtoms::flood_opacity, 
  
  &nsGkAtoms::font, 
  &nsGkAtoms::font_family, 
  &nsGkAtoms::font_size, 
  &nsGkAtoms::font_size_adjust, 
  &nsGkAtoms::font_stretch, 
  &nsGkAtoms::font_style, 
  &nsGkAtoms::font_variant, 
  &nsGkAtoms::fontWeight, 
  &nsGkAtoms::format, 
  &nsGkAtoms::from, 
  &nsGkAtoms::fx, 
  &nsGkAtoms::fy, 
  
  
  
  
  &nsGkAtoms::glyph_orientation_horizontal, 
  &nsGkAtoms::glyph_orientation_vertical, 
  &nsGkAtoms::gradientTransform, 
  &nsGkAtoms::gradientUnits, 
  &nsGkAtoms::height, 
  
  
  
  &nsGkAtoms::id, 
  
  &nsGkAtoms::image_rendering, 
  &nsGkAtoms::in, 
  &nsGkAtoms::in2, 
  &nsGkAtoms::intercept, 
  
  &nsGkAtoms::k1, 
  &nsGkAtoms::k2, 
  &nsGkAtoms::k3, 
  &nsGkAtoms::k4, 
  &nsGkAtoms::kerning, 
  &nsGkAtoms::kernelMatrix, 
  &nsGkAtoms::kernelUnitLength, 
  &nsGkAtoms::keyPoints, 
  &nsGkAtoms::keySplines, 
  &nsGkAtoms::keyTimes, 
  &nsGkAtoms::lang, 
  
  &nsGkAtoms::letter_spacing, 
  &nsGkAtoms::lighting_color, 
  &nsGkAtoms::limitingConeAngle, 
  
  &nsGkAtoms::marker, 
  &nsGkAtoms::marker_end, 
  &nsGkAtoms::marker_mid, 
  &nsGkAtoms::marker_start, 
  &nsGkAtoms::markerHeight, 
  &nsGkAtoms::markerUnits, 
  &nsGkAtoms::markerWidth, 
  &nsGkAtoms::mask, 
  &nsGkAtoms::maskContentUnits, 
  &nsGkAtoms::maskUnits, 
  
  &nsGkAtoms::max, 
  &nsGkAtoms::media, 
  &nsGkAtoms::method, 
  &nsGkAtoms::min, 
  &nsGkAtoms::mode, 
  &nsGkAtoms::name, 
  &nsGkAtoms::numOctaves, 
  &nsGkAtoms::offset, 
  &nsGkAtoms::opacity, 
  &nsGkAtoms::_operator, 
  &nsGkAtoms::order, 
  &nsGkAtoms::orient, 
  &nsGkAtoms::orientation, 
  
  
  
  &nsGkAtoms::overflow, 
  
  &nsGkAtoms::path, 
  &nsGkAtoms::pathLength, 
  &nsGkAtoms::patternContentUnits, 
  &nsGkAtoms::patternTransform, 
  &nsGkAtoms::patternUnits, 
  &nsGkAtoms::pointer_events, 
  &nsGkAtoms::points, 
  &nsGkAtoms::pointsAtX, 
  &nsGkAtoms::pointsAtY, 
  &nsGkAtoms::pointsAtZ, 
  &nsGkAtoms::preserveAlpha, 
  &nsGkAtoms::preserveAspectRatio, 
  &nsGkAtoms::primitiveUnits, 
  &nsGkAtoms::r, 
  &nsGkAtoms::radius, 
  &nsGkAtoms::refX, 
  &nsGkAtoms::refY, 
  &nsGkAtoms::repeatCount, 
  &nsGkAtoms::repeatDur, 
  &nsGkAtoms::requiredExtensions, 
  &nsGkAtoms::requiredFeatures, 
  &nsGkAtoms::restart, 
  &nsGkAtoms::result, 
  &nsGkAtoms::rotate, 
  &nsGkAtoms::rx, 
  &nsGkAtoms::ry, 
  &nsGkAtoms::scale, 
  &nsGkAtoms::seed, 
  &nsGkAtoms::shape_rendering, 
  &nsGkAtoms::slope, 
  &nsGkAtoms::spacing, 
  &nsGkAtoms::specularConstant, 
  &nsGkAtoms::specularExponent, 
  &nsGkAtoms::spreadMethod, 
  &nsGkAtoms::startOffset, 
  &nsGkAtoms::stdDeviation, 
  
  
  &nsGkAtoms::stitchTiles, 
  &nsGkAtoms::stop_color, 
  &nsGkAtoms::stop_opacity, 
  
  
  &nsGkAtoms::string, 
  &nsGkAtoms::stroke, 
  &nsGkAtoms::stroke_dasharray, 
  &nsGkAtoms::stroke_dashoffset, 
  &nsGkAtoms::stroke_linecap, 
  &nsGkAtoms::stroke_linejoin, 
  &nsGkAtoms::stroke_miterlimit, 
  &nsGkAtoms::stroke_opacity, 
  &nsGkAtoms::stroke_width, 
  &nsGkAtoms::surfaceScale, 
  &nsGkAtoms::systemLanguage, 
  &nsGkAtoms::tableValues, 
  &nsGkAtoms::target, 
  &nsGkAtoms::targetX, 
  &nsGkAtoms::targetY, 
  &nsGkAtoms::text_anchor, 
  &nsGkAtoms::text_decoration, 
  
  &nsGkAtoms::text_rendering, 
  &nsGkAtoms::title, 
  &nsGkAtoms::to, 
  &nsGkAtoms::transform, 
  &nsGkAtoms::type, 
  
  
  
  
  
  &nsGkAtoms::unicode_bidi, 
  
  
  
  
  
  
  &nsGkAtoms::values, 
  &nsGkAtoms::vector_effect, 
  
  
  
  &nsGkAtoms::viewBox, 
  &nsGkAtoms::viewTarget, 
  &nsGkAtoms::visibility, 
  &nsGkAtoms::width, 
  
  &nsGkAtoms::word_spacing, 
  
  &nsGkAtoms::x, 
  
  &nsGkAtoms::x1, 
  &nsGkAtoms::x2, 
  &nsGkAtoms::xChannelSelector, 
  &nsGkAtoms::y, 
  &nsGkAtoms::y1, 
  &nsGkAtoms::y2, 
  &nsGkAtoms::yChannelSelector, 
  &nsGkAtoms::z, 
  &nsGkAtoms::zoomAndPan, 
  nullptr
};

nsIAtom** const kURLAttributesSVG[] = {
  nullptr
};

nsIAtom** const kElementsMathML[] = {
   &nsGkAtoms::abs_, 
   &nsGkAtoms::_and, 
   &nsGkAtoms::annotation_, 
   &nsGkAtoms::annotation_xml_, 
   &nsGkAtoms::apply_, 
   &nsGkAtoms::approx_, 
   &nsGkAtoms::arccos_, 
   &nsGkAtoms::arccosh_, 
   &nsGkAtoms::arccot_, 
   &nsGkAtoms::arccoth_, 
   &nsGkAtoms::arccsc_, 
   &nsGkAtoms::arccsch_, 
   &nsGkAtoms::arcsec_, 
   &nsGkAtoms::arcsech_, 
   &nsGkAtoms::arcsin_, 
   &nsGkAtoms::arcsinh_, 
   &nsGkAtoms::arctan_, 
   &nsGkAtoms::arctanh_, 
   &nsGkAtoms::arg_, 
   &nsGkAtoms::bind_, 
   &nsGkAtoms::bvar_, 
   &nsGkAtoms::card_, 
   &nsGkAtoms::cartesianproduct_, 
   &nsGkAtoms::cbytes_, 
   &nsGkAtoms::ceiling, 
   &nsGkAtoms::cerror_, 
   &nsGkAtoms::ci_, 
   &nsGkAtoms::cn_, 
   &nsGkAtoms::codomain_, 
   &nsGkAtoms::complexes_, 
   &nsGkAtoms::compose_, 
   &nsGkAtoms::condition_, 
   &nsGkAtoms::conjugate_, 
   &nsGkAtoms::cos_, 
   &nsGkAtoms::cosh_, 
   &nsGkAtoms::cot_, 
   &nsGkAtoms::coth_, 
   &nsGkAtoms::cs_, 
   &nsGkAtoms::csc_, 
   &nsGkAtoms::csch_, 
   &nsGkAtoms::csymbol_, 
   &nsGkAtoms::curl_, 
   &nsGkAtoms::declare, 
   &nsGkAtoms::degree_, 
   &nsGkAtoms::determinant_, 
   &nsGkAtoms::diff_, 
   &nsGkAtoms::divergence_, 
   &nsGkAtoms::divide_, 
   &nsGkAtoms::domain_, 
   &nsGkAtoms::domainofapplication_, 
   &nsGkAtoms::el_, 
   &nsGkAtoms::emptyset_, 
   &nsGkAtoms::eq_, 
   &nsGkAtoms::equivalent_, 
   &nsGkAtoms::eulergamma_, 
   &nsGkAtoms::exists_, 
   &nsGkAtoms::exp_, 
   &nsGkAtoms::exponentiale_, 
   &nsGkAtoms::factorial_, 
   &nsGkAtoms::factorof_, 
   &nsGkAtoms::_false, 
   &nsGkAtoms::floor, 
   &nsGkAtoms::fn_, 
   &nsGkAtoms::forall_, 
   &nsGkAtoms::gcd_, 
   &nsGkAtoms::geq_, 
   &nsGkAtoms::grad, 
   &nsGkAtoms::gt_, 
   &nsGkAtoms::ident_, 
   &nsGkAtoms::image, 
   &nsGkAtoms::imaginary_, 
   &nsGkAtoms::imaginaryi_, 
   &nsGkAtoms::implies_, 
   &nsGkAtoms::in, 
   &nsGkAtoms::infinity, 
   &nsGkAtoms::int_, 
   &nsGkAtoms::integers_, 
   &nsGkAtoms::intersect_, 
   &nsGkAtoms::interval_, 
   &nsGkAtoms::inverse_, 
   &nsGkAtoms::lambda_, 
   &nsGkAtoms::laplacian_, 
   &nsGkAtoms::lcm_, 
   &nsGkAtoms::leq_, 
   &nsGkAtoms::limit_, 
   &nsGkAtoms::list_, 
   &nsGkAtoms::ln_, 
   &nsGkAtoms::log_, 
   &nsGkAtoms::logbase_, 
   &nsGkAtoms::lowlimit_, 
   &nsGkAtoms::lt_, 
   &nsGkAtoms::maction_, 
   &nsGkAtoms::maligngroup_, 
   &nsGkAtoms::malignmark_, 
   &nsGkAtoms::math, 
   &nsGkAtoms::matrix, 
   &nsGkAtoms::matrixrow_, 
   &nsGkAtoms::max, 
   &nsGkAtoms::mean_, 
   &nsGkAtoms::median_, 
   &nsGkAtoms::menclose_, 
   &nsGkAtoms::merror_, 
   &nsGkAtoms::mfenced_, 
   &nsGkAtoms::mfrac_, 
   &nsGkAtoms::mglyph_, 
   &nsGkAtoms::mi_, 
   &nsGkAtoms::min, 
   &nsGkAtoms::minus_, 
   &nsGkAtoms::mlabeledtr_, 
   &nsGkAtoms::mlongdiv_, 
   &nsGkAtoms::mmultiscripts_, 
   &nsGkAtoms::mn_, 
   &nsGkAtoms::mo_, 
   &nsGkAtoms::mode, 
   &nsGkAtoms::moment_, 
   &nsGkAtoms::momentabout_, 
   &nsGkAtoms::mover_, 
   &nsGkAtoms::mpadded_, 
   &nsGkAtoms::mphantom_, 
   &nsGkAtoms::mprescripts_, 
   &nsGkAtoms::mroot_, 
   &nsGkAtoms::mrow_, 
   &nsGkAtoms::ms_, 
   &nsGkAtoms::mscarries_, 
   &nsGkAtoms::mscarry_, 
   &nsGkAtoms::msgroup_, 
   &nsGkAtoms::msline_, 
   &nsGkAtoms::mspace_, 
   &nsGkAtoms::msqrt_, 
   &nsGkAtoms::msrow_, 
   &nsGkAtoms::mstack_, 
   &nsGkAtoms::mstyle_, 
   &nsGkAtoms::msub_, 
   &nsGkAtoms::msubsup_, 
   &nsGkAtoms::msup_, 
   &nsGkAtoms::mtable_, 
   &nsGkAtoms::mtd_, 
   &nsGkAtoms::mtext_, 
   &nsGkAtoms::mtr_, 
   &nsGkAtoms::munder_, 
   &nsGkAtoms::munderover_, 
   &nsGkAtoms::naturalnumbers_, 
   &nsGkAtoms::neq_, 
   &nsGkAtoms::none, 
   &nsGkAtoms::_not, 
   &nsGkAtoms::notanumber_, 
   &nsGkAtoms::note_, 
   &nsGkAtoms::notin_, 
   &nsGkAtoms::notprsubset_, 
   &nsGkAtoms::notsubset_, 
   &nsGkAtoms::_or, 
   &nsGkAtoms::otherwise, 
   &nsGkAtoms::outerproduct_, 
   &nsGkAtoms::partialdiff_, 
   &nsGkAtoms::pi_, 
   &nsGkAtoms::piece_, 
   &nsGkAtoms::piecewise_, 
   &nsGkAtoms::plus_, 
   &nsGkAtoms::power_, 
   &nsGkAtoms::primes_, 
   &nsGkAtoms::product_, 
   &nsGkAtoms::prsubset_, 
   &nsGkAtoms::quotient_, 
   &nsGkAtoms::rationals_, 
   &nsGkAtoms::real_, 
   &nsGkAtoms::reals_, 
   &nsGkAtoms::reln_, 
   &nsGkAtoms::rem, 
   &nsGkAtoms::root_, 
   &nsGkAtoms::scalarproduct_, 
   &nsGkAtoms::sdev_, 
   &nsGkAtoms::sec_, 
   &nsGkAtoms::sech_, 
   &nsGkAtoms::selector_, 
   &nsGkAtoms::semantics_, 
   &nsGkAtoms::sep_, 
   &nsGkAtoms::set_, 
   &nsGkAtoms::setdiff_, 
   &nsGkAtoms::share_, 
   &nsGkAtoms::sin_, 
   &nsGkAtoms::sinh_, 
   &nsGkAtoms::subset_, 
   &nsGkAtoms::sum, 
   &nsGkAtoms::tan_, 
   &nsGkAtoms::tanh_, 
   &nsGkAtoms::tendsto_, 
   &nsGkAtoms::times_, 
   &nsGkAtoms::transpose_, 
   &nsGkAtoms::_true, 
   &nsGkAtoms::union_, 
   &nsGkAtoms::uplimit_, 
   &nsGkAtoms::variance_, 
   &nsGkAtoms::vector_, 
   &nsGkAtoms::vectorproduct_, 
   &nsGkAtoms::xor_, 
  nullptr
};

nsIAtom** const kAttributesMathML[] = {
   &nsGkAtoms::accent_, 
   &nsGkAtoms::accentunder_, 
   &nsGkAtoms::actiontype_, 
   &nsGkAtoms::align, 
   &nsGkAtoms::alignmentscope_, 
   &nsGkAtoms::alt, 
   &nsGkAtoms::altimg_, 
   &nsGkAtoms::altimg_height_, 
   &nsGkAtoms::altimg_valign_, 
   &nsGkAtoms::altimg_width_, 
   &nsGkAtoms::background, 
   &nsGkAtoms::base, 
   &nsGkAtoms::bevelled_, 
   &nsGkAtoms::cd_, 
   &nsGkAtoms::cdgroup_, 
   &nsGkAtoms::charalign_, 
   &nsGkAtoms::close, 
   &nsGkAtoms::closure_, 
   &nsGkAtoms::color, 
   &nsGkAtoms::columnalign_, 
   &nsGkAtoms::columnalignment_, 
   &nsGkAtoms::columnlines_, 
   &nsGkAtoms::columnspacing_, 
   &nsGkAtoms::columnspan_, 
   &nsGkAtoms::columnwidth_, 
   &nsGkAtoms::crossout_, 
   &nsGkAtoms::decimalpoint_, 
   &nsGkAtoms::definitionURL_, 
   &nsGkAtoms::denomalign_, 
   &nsGkAtoms::depth_, 
   &nsGkAtoms::dir, 
   &nsGkAtoms::display, 
   &nsGkAtoms::displaystyle_, 
   &nsGkAtoms::edge_, 
   &nsGkAtoms::encoding, 
   &nsGkAtoms::equalcolumns_, 
   &nsGkAtoms::equalrows_, 
   &nsGkAtoms::fence_, 
   &nsGkAtoms::fontfamily_, 
   &nsGkAtoms::fontsize_, 
   &nsGkAtoms::fontstyle_, 
   &nsGkAtoms::fontweight_, 
   &nsGkAtoms::form, 
   &nsGkAtoms::frame, 
   &nsGkAtoms::framespacing_, 
   &nsGkAtoms::groupalign_, 
   &nsGkAtoms::height, 
   &nsGkAtoms::href, 
   &nsGkAtoms::id, 
   &nsGkAtoms::indentalign_, 
   &nsGkAtoms::indentalignfirst_, 
   &nsGkAtoms::indentalignlast_, 
   &nsGkAtoms::indentshift_, 
   &nsGkAtoms::indentshiftfirst_, 
   &nsGkAtoms::indenttarget_, 
   &nsGkAtoms::index, 
   &nsGkAtoms::integer, 
   &nsGkAtoms::largeop_, 
   &nsGkAtoms::length, 
   &nsGkAtoms::linebreak_, 
   &nsGkAtoms::linebreakmultchar_, 
   &nsGkAtoms::linebreakstyle_, 
   &nsGkAtoms::linethickness_, 
   &nsGkAtoms::location_, 
   &nsGkAtoms::longdivstyle_, 
   &nsGkAtoms::lquote_, 
   &nsGkAtoms::lspace_, 
   &nsGkAtoms::ltr, 
   &nsGkAtoms::mathbackground_, 
   &nsGkAtoms::mathcolor_, 
   &nsGkAtoms::mathsize_, 
   &nsGkAtoms::mathvariant_, 
   &nsGkAtoms::maxsize_, 
   &nsGkAtoms::minlabelspacing_, 
   &nsGkAtoms::minsize_, 
   &nsGkAtoms::movablelimits_, 
   &nsGkAtoms::msgroup_, 
   &nsGkAtoms::name, 
   &nsGkAtoms::newline, 
   &nsGkAtoms::notation_, 
   &nsGkAtoms::numalign_, 
   &nsGkAtoms::number, 
   &nsGkAtoms::open, 
   &nsGkAtoms::order, 
   &nsGkAtoms::other_, 
   &nsGkAtoms::overflow, 
   &nsGkAtoms::position, 
   &nsGkAtoms::role, 
   &nsGkAtoms::rowalign_, 
   &nsGkAtoms::rowlines_, 
   &nsGkAtoms::rowspacing_, 
   &nsGkAtoms::rowspan, 
   &nsGkAtoms::rquote_, 
   &nsGkAtoms::rspace_, 
   &nsGkAtoms::schemaLocation_, 
   &nsGkAtoms::scriptlevel_, 
   &nsGkAtoms::scriptminsize_, 
   &nsGkAtoms::scriptsize_, 
   &nsGkAtoms::scriptsizemultiplier_, 
   &nsGkAtoms::selection_, 
   &nsGkAtoms::separator_, 
   &nsGkAtoms::separators_, 
   &nsGkAtoms::shift_, 
   &nsGkAtoms::side_, 
   &nsGkAtoms::src, 
   &nsGkAtoms::stackalign_, 
   &nsGkAtoms::stretchy_, 
   &nsGkAtoms::subscriptshift_, 
   &nsGkAtoms::superscriptshift_, 
   &nsGkAtoms::symmetric_, 
   &nsGkAtoms::type, 
   &nsGkAtoms::voffset_, 
   &nsGkAtoms::width, 
   &nsGkAtoms::xref_, 
  nullptr
};

nsIAtom** const kURLAttributesMathML[] = {
  &nsGkAtoms::href,
  &nsGkAtoms::src,
  &nsGkAtoms::cdgroup_,
  &nsGkAtoms::altimg_,
  &nsGkAtoms::definitionURL_,
  nullptr
};

nsTHashtable<nsISupportsHashKey>* nsTreeSanitizer::sElementsHTML = nullptr;
nsTHashtable<nsISupportsHashKey>* nsTreeSanitizer::sAttributesHTML = nullptr;
nsTHashtable<nsISupportsHashKey>* nsTreeSanitizer::sPresAttributesHTML = nullptr;
nsTHashtable<nsISupportsHashKey>* nsTreeSanitizer::sElementsSVG = nullptr;
nsTHashtable<nsISupportsHashKey>* nsTreeSanitizer::sAttributesSVG = nullptr;
nsTHashtable<nsISupportsHashKey>* nsTreeSanitizer::sElementsMathML = nullptr;
nsTHashtable<nsISupportsHashKey>* nsTreeSanitizer::sAttributesMathML = nullptr;
nsIPrincipal* nsTreeSanitizer::sNullPrincipal = nullptr;

nsTreeSanitizer::nsTreeSanitizer(uint32_t aFlags)
 : mAllowStyles(aFlags & nsIParserUtils::SanitizerAllowStyle)
 , mAllowComments(aFlags & nsIParserUtils::SanitizerAllowComments)
 , mDropNonCSSPresentation(aFlags &
     nsIParserUtils::SanitizerDropNonCSSPresentation)
 , mDropForms(aFlags & nsIParserUtils::SanitizerDropForms)
 , mCidEmbedsOnly(aFlags &
     nsIParserUtils::SanitizerCidEmbedsOnly)
 , mDropMedia(aFlags & nsIParserUtils::SanitizerDropMedia)
 , mFullDocument(false)
{
  if (mCidEmbedsOnly) {
    
    mAllowStyles = false;
  }
  if (!sElementsHTML) {
    
    
    InitializeStatics();
  }
}

bool
nsTreeSanitizer::MustFlatten(int32_t aNamespace, nsIAtom* aLocal)
{
  if (aNamespace == kNameSpaceID_XHTML) {
    if (mDropNonCSSPresentation && (nsGkAtoms::font == aLocal ||
                                    nsGkAtoms::center == aLocal)) {
      return true;
    }
    if (mDropForms && (nsGkAtoms::form == aLocal ||
                       nsGkAtoms::input == aLocal ||
                       nsGkAtoms::keygen == aLocal ||
                       nsGkAtoms::option == aLocal ||
                       nsGkAtoms::optgroup == aLocal)) {
      return true;
    }
    if (mFullDocument && (nsGkAtoms::title == aLocal ||
                          nsGkAtoms::html == aLocal ||
                          nsGkAtoms::head == aLocal ||
                          nsGkAtoms::body == aLocal)) {
      return false;
    }
    return !sElementsHTML->GetEntry(aLocal);
  }
  if (aNamespace == kNameSpaceID_SVG) {
    if (mCidEmbedsOnly || mDropMedia) {
      
      
      return true;
    }
    return !sElementsSVG->GetEntry(aLocal);
  }
  if (aNamespace == kNameSpaceID_MathML) {
    return !sElementsMathML->GetEntry(aLocal);
  }
  return true;
}

bool
nsTreeSanitizer::IsURL(nsIAtom*** aURLs, nsIAtom* aLocalName)
{
  nsIAtom** atomPtrPtr;
  while ((atomPtrPtr = *aURLs)) {
    if (*atomPtrPtr == aLocalName) {
      return true;
    }
    ++aURLs;
  }
  return false;
}

bool
nsTreeSanitizer::MustPrune(int32_t aNamespace,
                           nsIAtom* aLocal,
                           mozilla::dom::Element* aElement)
{
  
  
  
  if (nsGkAtoms::script == aLocal) {
    return true;
  }
  if (aNamespace == kNameSpaceID_XHTML) {
    if (nsGkAtoms::title == aLocal && !mFullDocument) {
      
      return true;
    }
    if (mDropForms && (nsGkAtoms::select == aLocal ||
                       nsGkAtoms::button == aLocal ||
                       nsGkAtoms::datalist == aLocal)) {
      return true;
    }
    if (mDropMedia && (nsGkAtoms::img == aLocal ||
                       nsGkAtoms::video == aLocal ||
                       nsGkAtoms::audio == aLocal ||
                       nsGkAtoms::source == aLocal)) {
      return true;
    }
    if (nsGkAtoms::meta == aLocal &&
        (aElement->HasAttr(kNameSpaceID_None, nsGkAtoms::charset) ||
         aElement->HasAttr(kNameSpaceID_None, nsGkAtoms::httpEquiv))) {
      
      
      return true;
    }
    if (((!mFullDocument && nsGkAtoms::meta == aLocal) ||
        nsGkAtoms::link == aLocal) &&
        !(aElement->HasAttr(kNameSpaceID_None, nsGkAtoms::itemprop) ||
          aElement->HasAttr(kNameSpaceID_None, nsGkAtoms::itemscope))) {
      
      
      
      
      
      return true;
    }
  }
  if (mAllowStyles) {
    if (nsGkAtoms::style == aLocal && !(aNamespace == kNameSpaceID_XHTML
        || aNamespace == kNameSpaceID_SVG)) {
      return true;
    }
    return false;
  }
  if (nsGkAtoms::style == aLocal) {
    return true;
  }
  return false;
}

bool
nsTreeSanitizer::SanitizeStyleRule(mozilla::css::StyleRule *aRule,
                                   nsAutoString &aRuleText)
{
  bool didSanitize = false;
  aRuleText.Truncate();
  mozilla::css::Declaration* style = aRule->GetDeclaration();
  if (style) {
    didSanitize = style->HasProperty(eCSSProperty_binding);
    style->RemoveProperty(eCSSProperty_binding);
    style->ToString(aRuleText);
  }
  return didSanitize;
}

bool
nsTreeSanitizer::SanitizeStyleSheet(const nsAString& aOriginal,
                                    nsAString& aSanitized,
                                    nsIDocument* aDocument,
                                    nsIURI* aBaseURI)
{
  nsresult rv;
  aSanitized.Truncate();
  
  
  bool didSanitize = false;
  
  nsRefPtr<CSSStyleSheet> sheet = new CSSStyleSheet(CORS_NONE, aDocument->GetReferrerPolicy());
  sheet->SetURIs(aDocument->GetDocumentURI(), nullptr, aBaseURI);
  sheet->SetPrincipal(aDocument->NodePrincipal());
  
  nsCSSParser parser(nullptr, sheet);
  rv = parser.ParseSheet(aOriginal, aDocument->GetDocumentURI(), aBaseURI,
                         aDocument->NodePrincipal(), 0, false);
  NS_ENSURE_SUCCESS(rv, true);
  
  MOZ_ASSERT(!sheet->IsModified(),
             "should not get marked modified during parsing");
  sheet->SetComplete();
  
  int32_t ruleCount = sheet->StyleRuleCount();
  for (int32_t i = 0; i < ruleCount; ++i) {
    mozilla::css::Rule* rule = sheet->GetStyleRuleAt(i);
    if (!rule)
      continue;
    switch (rule->GetType()) {
      default:
        didSanitize = true;
        
        break;
      case mozilla::css::Rule::NAMESPACE_RULE:
      case mozilla::css::Rule::FONT_FACE_RULE: {
        
        nsAutoString cssText;
        nsCOMPtr<nsIDOMCSSRule> styleRule = do_QueryInterface(rule);
        if (styleRule) {
          rv = styleRule->GetCssText(cssText);
          if (NS_SUCCEEDED(rv)) {
            aSanitized.Append(cssText);
          }
        }
        break;
      }
      case mozilla::css::Rule::STYLE_RULE: {
        
        
        nsRefPtr<mozilla::css::StyleRule> styleRule = do_QueryObject(rule);
        NS_ASSERTION(styleRule, "Must be a style rule");
        nsAutoString decl;
        bool sanitized = SanitizeStyleRule(styleRule, decl);
        didSanitize = sanitized || didSanitize;
        if (!sanitized) {
          styleRule->GetCssText(decl);
        }
        aSanitized.Append(decl);
      }
    }
  }
  return didSanitize;
}

void
nsTreeSanitizer::SanitizeAttributes(mozilla::dom::Element* aElement,
                                    nsTHashtable<nsISupportsHashKey>* aAllowed,
                                    nsIAtom*** aURLs,
                                    bool aAllowXLink,
                                    bool aAllowStyle,
                                    bool aAllowDangerousSrc)
{
  uint32_t ac = aElement->GetAttrCount();

  nsresult rv;

  for (int32_t i = ac - 1; i >= 0; --i) {
    rv = NS_OK;
    const nsAttrName* attrName = aElement->GetAttrNameAt(i);
    int32_t attrNs = attrName->NamespaceID();
    nsCOMPtr<nsIAtom> attrLocal = attrName->LocalName();

    if (kNameSpaceID_None == attrNs) {
      if (aAllowStyle && nsGkAtoms::style == attrLocal) {
        nsCOMPtr<nsIURI> baseURI = aElement->GetBaseURI();
        nsIDocument* document = aElement->OwnerDoc();
        
        
        nsCSSParser parser(document->CSSLoader());
        nsRefPtr<mozilla::css::StyleRule> rule;
        nsAutoString value;
        aElement->GetAttr(attrNs, attrLocal, value);
        rv = parser.ParseStyleAttribute(value,
                                        document->GetDocumentURI(),
                                        baseURI,
                                        document->NodePrincipal(),
                                        getter_AddRefs(rule));
        if (NS_SUCCEEDED(rv)) {
          nsAutoString cleanValue;
          if (SanitizeStyleRule(rule, cleanValue)) {
            aElement->SetAttr(kNameSpaceID_None,
                              nsGkAtoms::style,
                              cleanValue,
                              false);
          }
        }
        continue;
      }
      if (aAllowDangerousSrc && nsGkAtoms::src == attrLocal) {
        continue;
      }
      if (IsURL(aURLs, attrLocal)) {
        if (SanitizeURL(aElement, attrNs, attrLocal)) {
          
          
          --ac;
          i = ac; 
          continue;
        }
        
        
        
      }
      if (!mDropNonCSSPresentation &&
          (aAllowed == sAttributesHTML) && 
          sPresAttributesHTML->GetEntry(attrLocal)) {
        continue;
      }
      if (aAllowed->GetEntry(attrLocal) &&
          !((attrLocal == nsGkAtoms::rel &&
             aElement->IsHTMLElement(nsGkAtoms::link)) ||
            (!mFullDocument &&
             attrLocal == nsGkAtoms::name &&
             aElement->IsHTMLElement(nsGkAtoms::meta)))) {
        
        
        
        
        
        continue;
      }
      const char16_t* localStr = attrLocal->GetUTF16String();
      
      
      if (*localStr == '_' || (attrLocal->GetLength() > 5 && localStr[0] == 'd'
          && localStr[1] == 'a' && localStr[2] == 't' && localStr[3] == 'a'
          && localStr[4] == '-')) {
        continue;
      }
      
    } else if (kNameSpaceID_XML == attrNs) {
      if (nsGkAtoms::base == attrLocal) {
        if (SanitizeURL(aElement, attrNs, attrLocal)) {
          
          
          --ac;
          i = ac; 
        }
        continue;
      }
      if (nsGkAtoms::lang == attrLocal || nsGkAtoms::space == attrLocal) {
        continue;
      }
      
    } else if (aAllowXLink && kNameSpaceID_XLink == attrNs) {
      if (nsGkAtoms::href == attrLocal) {
        if (SanitizeURL(aElement, attrNs, attrLocal)) {
          
          
          --ac;
          i = ac; 
        }
        continue;
      }
      if (nsGkAtoms::type == attrLocal || nsGkAtoms::title == attrLocal
          || nsGkAtoms::show == attrLocal || nsGkAtoms::actuate == attrLocal) {
        continue;
      }
      
    }
    aElement->UnsetAttr(kNameSpaceID_None, attrLocal, false);
    
    
    --ac;
    i = ac; 
  }

  
  
  if (aElement->IsAnyOfHTMLElements(nsGkAtoms::video, nsGkAtoms::audio)) {
    aElement->SetAttr(kNameSpaceID_None,
                      nsGkAtoms::controls,
                      EmptyString(),
                      false);
  }
}

bool
nsTreeSanitizer::SanitizeURL(mozilla::dom::Element* aElement,
                             int32_t aNamespace,
                             nsIAtom* aLocalName)
{
  nsAutoString value;
  aElement->GetAttr(aNamespace, aLocalName, value);

  
  static const char* kWhitespace = "\n\r\t\b";
  const nsAString& v =
    nsContentUtils::TrimCharsInSet(kWhitespace, value);

  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  uint32_t flags = nsIScriptSecurityManager::DISALLOW_INHERIT_PRINCIPAL;

  nsCOMPtr<nsIURI> baseURI = aElement->GetBaseURI();
  nsCOMPtr<nsIURI> attrURI;
  nsresult rv = NS_NewURI(getter_AddRefs(attrURI), v, nullptr, baseURI);
  if (NS_SUCCEEDED(rv)) { 
    if (mCidEmbedsOnly &&
        kNameSpaceID_None == aNamespace) {
      if (nsGkAtoms::src == aLocalName || nsGkAtoms::background == aLocalName) {
        
        
        
        if (!(v.Length() > 4 &&
              (v[0] == 'c' || v[0] == 'C') &&
              (v[1] == 'i' || v[1] == 'I') &&
              (v[2] == 'd' || v[2] == 'D') &&
              v[3] == ':')) {
          rv = NS_ERROR_FAILURE;
        }
      } else if (nsGkAtoms::cdgroup_ == aLocalName ||
                 nsGkAtoms::altimg_ == aLocalName ||
                 nsGkAtoms::definitionURL_ == aLocalName) {
        
        
        rv = NS_ERROR_FAILURE;
      } else {
        rv = secMan->CheckLoadURIWithPrincipal(sNullPrincipal, attrURI, flags);
      }
    } else {
      rv = secMan->CheckLoadURIWithPrincipal(sNullPrincipal, attrURI, flags);
    }
  }
  if (NS_FAILED(rv)) {
    aElement->UnsetAttr(aNamespace, aLocalName, false);
    return true;
  }
  return false;
}

void
nsTreeSanitizer::Sanitize(nsIContent* aFragment)
{
  
  
  
  NS_PRECONDITION(aFragment->IsNodeOfType(nsINode::eDOCUMENT_FRAGMENT),
      "Argument was not DOM fragment.");
  NS_PRECONDITION(!aFragment->IsInDoc(), "The fragment is in doc?");

  mFullDocument = false;
  SanitizeChildren(aFragment);
}

void
nsTreeSanitizer::Sanitize(nsIDocument* aDocument)
{
  
  
  
#ifdef DEBUG
  NS_PRECONDITION(!aDocument->GetContainer(), "The document is in a shell.");
  nsRefPtr<mozilla::dom::Element> root = aDocument->GetRootElement();
  NS_PRECONDITION(root->IsHTMLElement(nsGkAtoms::html), "Not HTML root.");
#endif

  mFullDocument = true;
  SanitizeChildren(aDocument);
}

void
nsTreeSanitizer::SanitizeChildren(nsINode* aRoot)
{
  nsIContent* node = aRoot->GetFirstChild();
  while (node) {
    if (node->IsElement()) {
      mozilla::dom::Element* elt = node->AsElement();
      mozilla::dom::NodeInfo* nodeInfo = node->NodeInfo();
      nsIAtom* localName = nodeInfo->NameAtom();
      int32_t ns = nodeInfo->NamespaceID();

      if (MustPrune(ns, localName, elt)) {
        RemoveAllAttributes(node);
        nsIContent* descendant = node;
        while ((descendant = descendant->GetNextNode(node))) {
          RemoveAllAttributes(descendant);
        }
        nsIContent* next = node->GetNextNonChildNode(aRoot);
        node->RemoveFromParent();
        node = next;
        continue;
      }
      if (nsGkAtoms::style == localName) {
        
        
        
        NS_ASSERTION(ns == kNameSpaceID_XHTML || ns == kNameSpaceID_SVG,
            "Should have only HTML or SVG here!");
        nsAutoString styleText;
        if (!nsContentUtils::GetNodeTextContent(node, false, styleText)) {
          NS_RUNTIMEABORT("OOM");
        }
        nsAutoString sanitizedStyle;
        nsCOMPtr<nsIURI> baseURI = node->GetBaseURI();
        if (SanitizeStyleSheet(styleText,
                               sanitizedStyle,
                               aRoot->OwnerDoc(),
                               baseURI)) {
          nsContentUtils::SetNodeTextContent(node, sanitizedStyle, true);
        } else {
          
          nsContentUtils::SetNodeTextContent(node, styleText, true);
        }
        if (ns == kNameSpaceID_XHTML) {
          SanitizeAttributes(elt,
                             sAttributesHTML,
                             (nsIAtom***)kURLAttributesHTML,
                             false,
                             mAllowStyles,
                             false);
        } else {
          SanitizeAttributes(elt,
                             sAttributesSVG,
                             (nsIAtom***)kURLAttributesSVG,
                             true,
                             mAllowStyles,
                             false);
        }
        node = node->GetNextNonChildNode(aRoot);
        continue;
      }
      if (MustFlatten(ns, localName)) {
        RemoveAllAttributes(node);
        nsIContent* next = node->GetNextNode(aRoot);
        nsIContent* parent = node->GetParent();
        nsCOMPtr<nsIContent> child; 
        ErrorResult rv;
        while ((child = node->GetFirstChild())) {
          parent->InsertBefore(*child, node, rv);
          if (rv.Failed()) {
            break;
          }
        }
        node->RemoveFromParent();
        node = next;
        continue;
      }
      NS_ASSERTION(ns == kNameSpaceID_XHTML ||
                   ns == kNameSpaceID_SVG ||
                   ns == kNameSpaceID_MathML,
          "Should have only HTML, MathML or SVG here!");
      if (ns == kNameSpaceID_XHTML) {
        SanitizeAttributes(elt,
                           sAttributesHTML,
                           (nsIAtom***)kURLAttributesHTML,
                           false, mAllowStyles,
                           (nsGkAtoms::img == localName) &&
                           !mCidEmbedsOnly);
      } else if (ns == kNameSpaceID_SVG) {
        SanitizeAttributes(elt,
                           sAttributesSVG,
                           (nsIAtom***)kURLAttributesSVG,
                           true,
                           mAllowStyles,
                           false);
      } else {
        SanitizeAttributes(elt,
                           sAttributesMathML,
                           (nsIAtom***)kURLAttributesMathML,
                           true,
                           false,
                           false);
      }
      node = node->GetNextNode(aRoot);
      continue;
    }
    NS_ASSERTION(!node->GetFirstChild(), "How come non-element node had kids?");
    nsIContent* next = node->GetNextNonChildNode(aRoot);
    if (!mAllowComments && node->IsNodeOfType(nsINode::eCOMMENT)) {
      node->RemoveFromParent();
    }
    node = next;
  }
}

void
nsTreeSanitizer::RemoveAllAttributes(nsIContent* aElement)
{
  const nsAttrName* attrName;
  while ((attrName = aElement->GetAttrNameAt(0))) {
    int32_t attrNs = attrName->NamespaceID();
    nsCOMPtr<nsIAtom> attrLocal = attrName->LocalName();
    aElement->UnsetAttr(attrNs, attrLocal, false);
  }
}

void
nsTreeSanitizer::InitializeStatics()
{
  NS_PRECONDITION(!sElementsHTML, "Initializing a second time.");

  sElementsHTML = new nsTHashtable<nsISupportsHashKey>(ArrayLength(kElementsHTML));
  for (uint32_t i = 0; kElementsHTML[i]; i++) {
    sElementsHTML->PutEntry(*kElementsHTML[i]);
  }

  sAttributesHTML = new nsTHashtable<nsISupportsHashKey>(ArrayLength(kAttributesHTML));
  for (uint32_t i = 0; kAttributesHTML[i]; i++) {
    sAttributesHTML->PutEntry(*kAttributesHTML[i]);
  }

  sPresAttributesHTML = new nsTHashtable<nsISupportsHashKey>(ArrayLength(kPresAttributesHTML));
  for (uint32_t i = 0; kPresAttributesHTML[i]; i++) {
    sPresAttributesHTML->PutEntry(*kPresAttributesHTML[i]);
  }

  sElementsSVG = new nsTHashtable<nsISupportsHashKey>(ArrayLength(kElementsSVG));
  for (uint32_t i = 0; kElementsSVG[i]; i++) {
    sElementsSVG->PutEntry(*kElementsSVG[i]);
  }

  sAttributesSVG = new nsTHashtable<nsISupportsHashKey>(ArrayLength(kAttributesSVG));
  for (uint32_t i = 0; kAttributesSVG[i]; i++) {
    sAttributesSVG->PutEntry(*kAttributesSVG[i]);
  }

  sElementsMathML = new nsTHashtable<nsISupportsHashKey>(ArrayLength(kElementsMathML));
  for (uint32_t i = 0; kElementsMathML[i]; i++) {
    sElementsMathML->PutEntry(*kElementsMathML[i]);
  }

  sAttributesMathML = new nsTHashtable<nsISupportsHashKey>(ArrayLength(kAttributesMathML));
  for (uint32_t i = 0; kAttributesMathML[i]; i++) {
    sAttributesMathML->PutEntry(*kAttributesMathML[i]);
  }

  nsCOMPtr<nsIPrincipal> principal =
      do_CreateInstance(NS_NULLPRINCIPAL_CONTRACTID);
  principal.forget(&sNullPrincipal);
}

void
nsTreeSanitizer::ReleaseStatics()
{
  delete sElementsHTML;
  sElementsHTML = nullptr;

  delete sAttributesHTML;
  sAttributesHTML = nullptr;

  delete sPresAttributesHTML;
  sPresAttributesHTML = nullptr;

  delete sElementsSVG;
  sElementsSVG = nullptr;

  delete sAttributesSVG;
  sAttributesSVG = nullptr;

  delete sElementsMathML;
  sElementsMathML = nullptr;

  delete sAttributesMathML;
  sAttributesMathML = nullptr;

  NS_IF_RELEASE(sNullPrincipal);
}
