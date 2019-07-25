




































































































void
SetupTests()
{
    TestEntry *t;

    
    gfxFontStyle style_western_normal_16 (FONT_STYLE_NORMAL,
                                          NS_FONT_STRETCH_NORMAL,
                                          400,
                                          16.0,
                                          NS_NewPermanentAtom(NS_LITERAL_STRING("en")),
                                          0.0,
                                          false, false, false,
                                          NS_LITERAL_STRING(""),
                                          NS_LITERAL_STRING(""));

    gfxFontStyle style_western_bold_16 (FONT_STYLE_NORMAL,
                                        NS_FONT_STRETCH_NORMAL,
                                        700,
                                        16.0,
                                        NS_NewPermanentAtom(NS_LITERAL_STRING("en")),
                                        0.0,
                                        false, false, false,
                                        NS_LITERAL_STRING(""),
                                        NS_LITERAL_STRING(""));

    
    t = AddTest ("sans-serif",
                 style_western_normal_16,
                 S_ASCII,
                 "ABCD");

    t->Expect ("win32", "Arial", GLYPHS(36, 37, 38, 39));
    t->Expect ("macosx", "Helvetica", GLYPHS(36, 37, 38, 39));
    t->Expect ("gtk2-pango", "Albany AMT", GLYPHS(36, 37, 38, 39));

    
    t = AddTest ("verdana,sans-serif",
                 style_western_normal_16,
                 S_UTF8,
                 "foo\xe2\x80\x91""bar");

    t->Expect ("win32", "Verdana", GLYPHS(73, 82, 82));
    t->Expect ("win32", "Arial Unicode MS", GLYPHS(3236));
    t->Expect ("win32", "Verdana", GLYPHS(69, 68, 85));

    t->Expect ("macosx", "Verdana", GLYPHS(73, 82, 82));
    t->Expect ("macosx", "Helvetica", GLYPHS(587));
    t->Expect ("macosx", "Verdana", GLYPHS(69, 68, 85));

    
    t = AddTest ("sans-serif",
                 style_western_bold_16,
                 S_ASCII,
                 "ABCD");

    t->Expect ("win32", "Arial:700", GLYPHS(36, 37, 38, 39));
    t->Expect ("macosx", "Helvetica-Bold", GLYPHS(36, 37, 38, 39));
    t->Expect ("gtk2-pango", "Albany AMT Bold", GLYPHS(36, 37, 38, 39));

    
    t = AddTest ("sans-serif",
                 style_western_normal_16,
                 S_UTF8,
                 " \xd8\xaa\xd9\x85 ");
    t->SetRTL();
    t->Expect ("macosx", "Helvetica", GLYPHS(3));
    t->Expect ("macosx", "AlBayan", GLYPHS(47));
    t->Expect ("macosx", "Helvetica", GLYPHS(3));
    t->Expect ("win32", "Arial", GLYPHS(3, 919, 994, 3));

    
    t = AddTest ("sans-serif",
                 style_western_normal_16,
                 S_UTF8,
                 " \xd9\x85\xd8\xaa ");
    t->Expect ("macosx", "Helvetica", GLYPHS(3));
    t->Expect ("macosx", "AlBayan", GLYPHS(2, 47));
    t->Expect ("macosx", "Helvetica", GLYPHS(3));
    t->Expect ("win32", "Arial", GLYPHS(3, 994, 919, 3));

    
    t = AddTest ("sans-serif",
                 style_western_normal_16,
                 S_ASCII,
                 " ab");
    t->SetRTL();
    t->Expect ("macosx", "Helvetica", GLYPHS(3, 68, 69));
    t->Expect ("win32", "Arial", GLYPHS(3, 68, 69));
    t->Expect ("gtk2-pango", "Albany AMT", GLYPHS(3, 68, 69));

    
    t = AddTest ("sans-serif",
                 style_western_normal_16,
                 S_ASCII,
                 "ab ");
    t->SetRTL();
    t->Expect ("macosx", "Helvetica", GLYPHS(68, 69, 3));
    t->Expect ("win32", "Arial", GLYPHS(68, 69, 3));
    t->Expect ("gtk2-pango", "Albany AMT", GLYPHS(68, 69, 3));

    
    
    t = AddTest ("sans-serif",
                 style_western_normal_16,
                 S_ASCII,
                 "fi");
    t->Expect ("macosx", "Helvetica", GLYPHS(192));
    t->Expect ("win32", "Arial", GLYPHS(73, 76));

    
    
    t = AddTest ("sans-serif",
                 style_western_normal_16,
                 S_UTF8,
                 "\xe0\xa4\x9a\xe0\xa4\xbe\xe0\xa4\xb9\xe0\xa4\xbf\xe0\xa4\x8f"); 
    t->Expect ("macosx", "DevanagariMT", GLYPHS(71, 100, 101, 99, 60));
    t->Expect ("win32", "Mangal", GLYPHS(133, 545, 465, 161, 102));

    
    t = AddTest ("Kartika",
                 style_western_normal_16,
                 S_UTF8,
                 "\xe0\xb4\xb3\xe0\xb5\x8d\xe2\x80\x8d");
    t->Expect ("win32", "Kartika", GLYPHS(332));

    
    




    t = AddTest ("sans-serif",
                 style_western_normal_16,
                 S_UTF8,
                 "\xe0\xb4\xb3\xe0\xb5\x8d\xe2\x80\x8d");
    t->Expect ("win32", "Kartika", GLYPHS(332));
}
