








































var gPropList =
{
  
  

  
  
  clip:              new AdditiveAttribute("clip", "CSS", "marker"),
  clip_path:         new NonAdditiveAttribute("clip-path", "CSS", "rect"),
  clip_rule:         new NonAdditiveAttribute("clip-rule", "CSS", "circle"),
  color:             new AdditiveAttribute("color", "CSS", "rect"),
  color_interpolation:
    new NonAdditiveAttribute("color-interpolation", "CSS", "rect"),
  color_interpolation_filters:
    new NonAdditiveAttribute("color-interpolation-filters", "CSS",
                             "feFlood"),
  
  
  cursor:            new NonAdditiveAttribute("cursor", "CSS", "rect"),
  direction:         new NonAnimatableAttribute("direction", "CSS", "text"),
  display:           new NonAdditiveAttribute("display", "CSS", "rect"),
  dominant_baseline:
    new NonAdditiveAttribute("dominant-baseline", "CSS", "text"),
  enable_background:
    
    new NonAnimatableAttribute("enable-background", "CSS", "marker"),
  fill:              new AdditiveAttribute("fill", "CSS", "rect"),
  fill_opacity:      new AdditiveAttribute("fill-opacity", "CSS", "rect"),
  fill_rule:         new NonAdditiveAttribute("fill-rule", "CSS", "rect"),
  filter:            new NonAdditiveAttribute("filter", "CSS", "rect"),
  flood_color:       new AdditiveAttribute("flood-color", "CSS", "feFlood"),
  flood_opacity:     new AdditiveAttribute("flood-opacity", "CSS", "feFlood"),
  font:              new NonAdditiveAttribute("font", "CSS", "text"),
  font_family:       new NonAdditiveAttribute("font-family", "CSS", "text"),
  font_size:         new AdditiveAttribute("font-size", "CSS", "text"),
  font_size_adjust:
    new NonAdditiveAttribute("font-size-adjust", "CSS", "text"),
  font_stretch:      new NonAdditiveAttribute("font-stretch", "CSS", "text"),
  font_style:        new NonAdditiveAttribute("font-style", "CSS", "text"),
  font_variant:      new NonAdditiveAttribute("font-variant", "CSS", "text"),
  
  font_weight:       new NonAdditiveAttribute("font-weight", "CSS", "text"),
  glyph_orientation_horizontal:
    
    NonAnimatableAttribute("glyph-orientation-horizontal", "CSS", "text"),
  glyph_orientation_vertical:
    
    NonAnimatableAttribute("glyph-orientation-horizontal", "CSS", "text"),
  image_rendering:
    NonAdditiveAttribute("image-rendering", "CSS", "image"),
  
  letter_spacing:    new AdditiveAttribute("letter-spacing", "CSS", "text"),
  lighting_color:
    new AdditiveAttribute("lighting-color", "CSS", "feDiffuseLighting"),
  marker:            new NonAdditiveAttribute("marker", "CSS", "line"),
  marker_end:        new NonAdditiveAttribute("marker-end", "CSS", "line"),
  marker_mid:        new NonAdditiveAttribute("marker-mid", "CSS", "line"),
  marker_start:      new NonAdditiveAttribute("marker-start", "CSS", "line"),
  mask:              new NonAdditiveAttribute("mask", "CSS", "line"),
  opacity:           new AdditiveAttribute("opacity", "CSS", "rect"),
  overflow:          new NonAdditiveAttribute("overflow", "CSS", "marker"),
  pointer_events:    new NonAdditiveAttribute("pointer-events", "CSS", "rect"),
  shape_rendering:   new NonAdditiveAttribute("shape-rendering", "CSS", "rect"),
  stop_color:        new AdditiveAttribute("stop-color", "CSS", "stop"),
  stop_opacity:      new AdditiveAttribute("stop-opacity", "CSS", "stop"),
  stroke:            new AdditiveAttribute("stroke", "CSS", "rect"),
  stroke_dasharray:  new NonAdditiveAttribute("stroke-dasharray", "CSS", "rect"),
  stroke_dashoffset: new AdditiveAttribute("stroke-dashoffset", "CSS", "rect"),
  stroke_linecap:    new NonAdditiveAttribute("stroke-linecap", "CSS", "rect"),
  stroke_linejoin:   new NonAdditiveAttribute("stroke-linejoin", "CSS", "rect"),
  stroke_miterlimit: new AdditiveAttribute("stroke-miterlimit", "CSS", "rect"),
  stroke_opacity:    new AdditiveAttribute("stroke-opacity", "CSS", "rect"),
  stroke_width:      new AdditiveAttribute("stroke-width", "CSS", "rect"),
  text_anchor:       new NonAdditiveAttribute("text-anchor", "CSS", "text"),
  text_decoration:   new NonAdditiveAttribute("text-decoration", "CSS", "text"),
  text_rendering:    new NonAdditiveAttribute("text-rendering", "CSS", "text"),
  unicode_bidi:      new NonAnimatableAttribute("unicode-bidi", "CSS", "text"),
  visibility:        new NonAdditiveAttribute("visibility", "CSS", "rect"),
  word_spacing:      new AdditiveAttribute("word-spacing", "CSS", "text"),
  writing_mode:
    
    new NonAnimatableAttribute("writing-mode", "CSS", "text"),
};
