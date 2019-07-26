




















#include <stdio.h>

static int
png_have_neon(png_structp png_ptr)
{
   FILE *f = fopen("/proc/cpuinfo", "rb");

   if (f != NULL)
   {
      


      static const char ch_feature[] = { 70, 69, 65, 84, 85, 82, 69, 83 };
      static const char ch_neon[] = { 78, 69, 79, 78 };

      enum
      {
         StartLine, Feature, Colon, StartTag, Neon, HaveNeon, SkipTag, SkipLine
      }  state;
      int counter;

      for (state=StartLine, counter=0;;)
      {
         int ch = fgetc(f);

         if (ch == EOF)
         {
            


            fclose(f);
            return 0;
         }

         switch (state)
         {
            case StartLine:
               
               if (ch <= 32) 
                  break;

               counter=0;
               state = Feature;
               

            case Feature:
               
               if ((ch & ~0x20) == ch_feature[counter])
               {
                  if (++counter == (sizeof ch_feature))
                     state = Colon;
                  break;
               }
               
               
               state = SkipLine;
               

            case SkipLine:
            skipLine:
               
               if (ch != 10 && ch != 13)
                  break;

               state = StartLine;
               break;

            case Colon:
               
               if (ch == 32 || ch == 9)
                  break;

               if (ch == 58) 
               {
                  state = StartTag;
                  break;
               }

               


               state = SkipLine;
               goto skipLine;

            case StartTag:
               
               if (ch == 32 || ch == 9)
                  break;

               state = Neon;
               counter = 0;
               

            case Neon:
               
               if ((ch & ~0x20) == ch_neon[counter])
               {
                  if (++counter == (sizeof ch_neon))
                     state = HaveNeon;
                  break;
               }

               state = SkipTag;
               

            case SkipTag:
               
               if (ch == 10 || ch == 13)
                  state = StartLine;

               else if (ch == 32 || ch == 9)
                  state = StartTag;
               break;

            case HaveNeon:
               


               if (ch == 10 || ch == 13 || ch == 32 || ch == 9)
               {
                  fclose(f);
                  return 1;
               }

               state = SkipTag;
               break;

            default:
               png_error(png_ptr, "png_have_neon: internal error (bug)");
         }
      }
   }

   else
      png_warning(png_ptr, "/proc/cpuinfo open failed");

   return 0;
}
