






















































  
  
  

  




#define FEATREG_MAX_SETTING  12

  
  
  


#include <stdio.h>
#include <string.h>


  
  
  
  
  
  
  

#define APPLE_RESERVED         "Apple Reserved"
#define APPLE_RESERVED_LENGTH  14

  typedef struct  GX_Feature_RegistryRec_
  {
    const char*  feat_name;
    char         exclusive;
    char*        setting_name[FEATREG_MAX_SETTING];

  } GX_Feature_RegistryRec;


#define EMPTYFEAT {0, 0, {NULL}}


  static GX_Feature_RegistryRec featreg_table[] = {
    {                                       
      "All Typographic Features",
      0,
      {
        "All Type Features",
        NULL
      }
    }, {                                    
      "Ligatures",
      0,
      {
        "Required Ligatures",
        "Common Ligatures",
        "Rare Ligatures",
        "Logos",
        "Rebus Pictures",
        "Diphthong Ligatures",
        "Squared Ligatures",
        "Squared Ligatures, Abbreviated",
        NULL
      }
    }, {                                    
      "Cursive Connection",
      1,
      {
        "Unconnected",
        "Partially Connected",
        "Cursive",
        NULL
      }
    }, {                                    
      "Letter Case",
      1,
      {
        "Upper & Lower Case",
        "All Caps",
        "All Lower Case",
        "Small Caps",
        "Initial Caps",
        "Initial Caps & Small Caps",
        NULL
      }
    }, {                                    
      "Vertical Substitution",
      0,
      {
        
        "Turns on the feature",
        NULL
      }
    }, {                                    
      "Linguistic Rearrangement",
      0,
      {
        
        "Turns on the feature",
        NULL
      }
    }, {                                    
      "Number Spacing",
      1,
      {
        "Monospaced Numbers",
        "Proportional Numbers",
        NULL
      }
    }, {                                    
      APPLE_RESERVED " 1",
      0,
      {NULL}
    }, {                                    
      "Smart Swashes",
      0,
      {
        "Word Initial Swashes",
        "Word Final Swashes",
        "Line Initial Swashes",
        "Line Final Swashes",
        "Non-Final Swashes",
        NULL
      }
    }, {                                    
      "Diacritics",
      1,
      {
        "Show Diacritics",
        "Hide Diacritics",
        "Decompose Diacritics",
        NULL
      }
    }, {                                    
      "Vertical Position",
      1,
      {
        
        "No Vertical Position",
        "Superiors",
        "Inferiors",
        "Ordinals",
        NULL
      }
    }, {                                    
      "Fractions",
      1,
      {
        "No Fractions",
        "Vertical Fractions",
        "Diagonal Fractions",
        NULL
      }
    }, {                                    
      APPLE_RESERVED " 2",
      0,
      {NULL}
    }, {                                    
      "Overlapping Characters",
      0,
      {
        
        "Turns on the feature",
        NULL
      }
    }, {                                    
      "Typographic Extras",
      0,
      {
        "Hyphens to Em Dash",
        "Hyphens to En Dash",
        "Unslashed Zero",
        "Form Interrobang",
        "Smart Quotes",
        "Periods to Ellipsis",
        NULL
      }
    }, {                                    
      "Mathematical Extras",
      0,
      {
        "Hyphens to Minus",
        "Asterisk to Multiply",
        "Slash to Divide",
        "Inequality Ligatures",
        "Exponents",
        NULL
      }
    }, {                                    
      "Ornament Sets",
      1,
      {
        "No Ornaments",
        "Dingbats",
        "Pi Characters",
        "Fleurons",
        "Decorative Borders",
        "International Symbols",
        "Math Symbols",
        NULL
      }
    }, {                                    
      "Character Alternatives",
      1,
      {
        "No Alternates",
        
        NULL
      }
    }, {                                    
      "Design Complexity",
      1,
      {
        "Design Level 1",
        "Design Level 2",
        "Design Level 3",
        "Design Level 4",
        "Design Level 5",
        
        NULL
      }
    }, {                                    
      "Style Options",
      1,
      {
        "No Style Options",
        "Display Text",
        "Engraved Text",
        "Illuminated Caps",
        "Tilling Caps",
        "Tall Caps",
        NULL
      }
    }, {                                    
      "Character Shape",
      1,
      {
        "Traditional Characters",
        "Simplified Characters",
        "JIS 1978 Characters",
        "JIS 1983 Characters",
        "JIS 1990 Characters",
        "Traditional Characters, Alternative Set 1",
        "Traditional Characters, Alternative Set 2",
        "Traditional Characters, Alternative Set 3",
        "Traditional Characters, Alternative Set 4",
        "Traditional Characters, Alternative Set 5",
        "Expert Characters",
        NULL                           
      }
    }, {                                    
      "Number Case",
      1,
      {
        "Lower Case Numbers",
        "Upper Case Numbers",
        NULL
      }
    }, {                                    
      "Text Spacing",
      1,
      {
        "Proportional",
        "Monospaced",
        "Half-width",
        "Normal",
        NULL
      }
    },   { 
      "Transliteration",
      1,
      {
        "No Transliteration",
        "Hanja To Hangul",
        "Hiragana to Katakana",
        "Katakana to Hiragana",
        "Kana to Romanization",
        "Romanization to Hiragana",
        "Romanization to Katakana",
        "Hanja to Hangul, Alternative Set 1",
        "Hanja to Hangul, Alternative Set 2",
        "Hanja to Hangul, Alternative Set 3",
        NULL
      }
    }, {                                    
      "Annotation",
      1,
      {
        "No Annotation",
        "Box Annotation",
        "Rounded Box Annotation",
        "Circle Annotation",
        "Inverted Circle Annotation",
        "Parenthesis Annotation",
        "Period Annotation",
        "Roman Numeral Annotation",
        "Diamond Annotation",
        NULL
      }
    }, {                                    
      "Kana Spacing",
      1,
      {
        "Full Width",
        "Proportional",
        NULL
      }
    }, {                                    
      "Ideographic Spacing",
      1,
      {
        "Full Width",
        "Proportional",
        NULL
      }
    }, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT,         
    EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, 
    EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, 
    EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, 
    EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, 
    EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, 
    EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, 
    EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, 
    EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, 
    EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, 
    EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, 
    EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, 
    EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, 
    EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, EMPTYFEAT, 
    EMPTYFEAT, EMPTYFEAT, EMPTYFEAT,                       
    EMPTYFEAT,  {                   
      "Text Spacing",
      1,
      {
        "Proportional",
        "Monospaced",
        "Half-width",
        "Normal",
        NULL
      }
    }, {                                    
      "Kana Spacing",
      1,
      {
        "Full Width",
        "Proportional",
        NULL
      }
    }, {                                    
      "Ideographic Spacing",
      1,
      {
        "Full Width",
        "Proportional",
        NULL
      }
    }, {                                    
      "CJK Roman Spacing",
      1,
      {
        "Half-width",
        "Proportional",
        "Default Roman",
        "Full-width Roman",
        NULL
      }
    }, {                                    
      "All Typographic Features",
      0,
      {
        "All Type Features",
        NULL
      }
    }
  };


  
  
  
  
  
  
  

  int
  main( void )
  {
    int  i;


    printf( "  {\n" );
    printf( "   /* Generated from %s */\n", __FILE__ );

    for ( i = 0;
          i < sizeof ( featreg_table ) / sizeof ( GX_Feature_RegistryRec );
          i++ )
    {
      const char*  feat_name;
      int          nSettings;


      feat_name = featreg_table[i].feat_name;
      for ( nSettings = 0;
            featreg_table[i].setting_name[nSettings];
            nSettings++)
        ;                                   

      printf( "    {%1d, %1d, %1d, %2d},   /* %s */\n",
              feat_name ? 1 : 0,
              ( feat_name                                                  &&
                ( ft_strncmp( feat_name,
                              APPLE_RESERVED, APPLE_RESERVED_LENGTH ) == 0 )
              ) ? 1 : 0,
              featreg_table[i].exclusive ? 1 : 0,
              nSettings,
              feat_name ? feat_name : "__EMPTY__" );
    }

    printf( "  };\n" );

    return 0;
  }



