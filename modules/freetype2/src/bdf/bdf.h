























#ifndef __BDF_H__
#define __BDF_H__






#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_STREAM_H


FT_BEGIN_HEADER




#define _bdf_glyph_modified( map, e )                 \
          ( (map)[(e) >> 5] & ( 1 << ( (e) & 31 ) ) )
#define _bdf_set_glyph_modified( map, e )              \
          ( (map)[(e) >> 5] |= ( 1 << ( (e) & 31 ) ) )
#define _bdf_clear_glyph_modified( map, e )             \
          ( (map)[(e) >> 5] &= ~( 1 << ( (e) & 31 ) ) )




  
  
  
  
  


#define BDF_CORRECT_METRICS  0x01 /* Correct invalid metrics when loading. */
#define BDF_KEEP_COMMENTS    0x02 /* Preserve the font comments.           */
#define BDF_KEEP_UNENCODED   0x04 /* Keep the unencoded glyphs.            */
#define BDF_PROPORTIONAL     0x08 /* Font has proportional spacing.        */
#define BDF_MONOWIDTH        0x10 /* Font has mono width.                  */
#define BDF_CHARCELL         0x20 /* Font has charcell spacing.            */

#define BDF_ALL_SPACING  ( BDF_PROPORTIONAL | \
                           BDF_MONOWIDTH    | \
                           BDF_CHARCELL     )

#define BDF_DEFAULT_LOAD_OPTIONS  ( BDF_CORRECT_METRICS | \
                                    BDF_KEEP_COMMENTS   | \
                                    BDF_KEEP_UNENCODED  | \
                                    BDF_PROPORTIONAL    )


  typedef struct  bdf_options_t_
  {
    int            correct_metrics;
    int            keep_unencoded;
    int            keep_comments;
    int            font_spacing;

  } bdf_options_t;


  
  typedef int
  (*bdf_options_callback_t)( bdf_options_t*  opts,
                             char**          params,
                             unsigned long   nparams,
                             void*           client_data );


  
  
  
  
  


#define BDF_ATOM      1
#define BDF_INTEGER   2
#define BDF_CARDINAL  3


  
  
  typedef struct  bdf_property_t_
  {
    char*  name;         
    int    format;       
    int    builtin;      
    union
    {
      char*          atom;
      long           l;
      unsigned long  ul;

    } value;             

  } bdf_property_t;


  
  
  
  
  


  typedef struct  bdf_bbx_t_
  {
    unsigned short  width;
    unsigned short  height;

    short           x_offset;
    short           y_offset;

    short           ascent;
    short           descent;

  } bdf_bbx_t;


  typedef struct  bdf_glyph_t_
  {
    char*           name;        
    long            encoding;    
    unsigned short  swidth;      
    unsigned short  dwidth;      
    bdf_bbx_t       bbx;         
    unsigned char*  bitmap;      
    unsigned long   bpr;         
    unsigned short  bytes;       

  } bdf_glyph_t;


  typedef struct  _hashnode_
  {
    const char*  key;
    size_t       data;

  } _hashnode, *hashnode;


  typedef struct  hashtable_
  {
    int        limit;
    int        size;
    int        used;
    hashnode*  table;

  } hashtable;


  typedef struct  bdf_glyphlist_t_
  {
    unsigned short  pad;          
    unsigned short  bpp;          
    long            start;        
    long            end;          
    bdf_glyph_t*    glyphs;       
    unsigned long   glyphs_size;  
    unsigned long   glyphs_used;  
    bdf_bbx_t       bbx;          

  } bdf_glyphlist_t;


  typedef struct  bdf_font_t_
  {
    char*            name;           
    bdf_bbx_t        bbx;            

    long             point_size;     
    unsigned long    resolution_x;   
    unsigned long    resolution_y;   

    int              spacing;        

    unsigned short   monowidth;      

    long             default_char;   

    long             font_ascent;    
    long             font_descent;   

    unsigned long    glyphs_size;    
    unsigned long    glyphs_used;    
    bdf_glyph_t*     glyphs;         

    unsigned long    unencoded_size; 
    unsigned long    unencoded_used; 
    bdf_glyph_t*     unencoded;      

    unsigned long    props_size;     
    unsigned long    props_used;     
    bdf_property_t*  props;          

    char*            comments;       
    unsigned long    comments_len;   

    bdf_glyphlist_t  overflow;       

    void*            internal;       

    
    
    unsigned long    nmod[34816];    
    unsigned long    umod[34816];    
                                     
    unsigned short   modified;       
    unsigned short   bpp;            

    FT_Memory        memory;

    bdf_property_t*  user_props;
    unsigned long    nuser_props;
    hashtable        proptbl;

  } bdf_font_t;


  
  
  
  
  


  
#define BDF_MISSING_START       -1
#define BDF_MISSING_FONTNAME    -2
#define BDF_MISSING_SIZE        -3
#define BDF_MISSING_CHARS       -4
#define BDF_MISSING_STARTCHAR   -5
#define BDF_MISSING_ENCODING    -6
#define BDF_MISSING_BBX         -7

#define BDF_OUT_OF_MEMORY      -20

#define BDF_INVALID_LINE      -100


  
  
  
  
  

  FT_LOCAL( FT_Error )
  bdf_load_font( FT_Stream       stream,
                 FT_Memory       memory,
                 bdf_options_t*  opts,
                 bdf_font_t*    *font );

  FT_LOCAL( void )
  bdf_free_font( bdf_font_t*  font );

  FT_LOCAL( bdf_property_t * )
  bdf_get_property( char*        name,
                    bdf_font_t*  font );

  FT_LOCAL( bdf_property_t * )
  bdf_get_font_property( bdf_font_t*  font,
                         const char*  name );


FT_END_HEADER


#endif 



