






































#include "nsCSSColorUtils.h"
#include <math.h>




#define RED_LUMINOSITY        299
#define GREEN_LUMINOSITY      587
#define BLUE_LUMINOSITY       114
#define INTENSITY_FACTOR      25
#define LIGHT_FACTOR          0
#define LUMINOSITY_FACTOR     75

#define MAX_COLOR             255
#define COLOR_DARK_THRESHOLD  51
#define COLOR_LIGHT_THRESHOLD 204

#define COLOR_LITE_BS_FACTOR 45
#define COLOR_LITE_TS_FACTOR 70

#define COLOR_DARK_BS_FACTOR 30
#define COLOR_DARK_TS_FACTOR 50

#define LIGHT_GRAY NS_RGB(192, 192, 192)
#define DARK_GRAY  NS_RGB(96, 96, 96)
#define WHITE      NS_RGB(255, 255, 255)
#define BLACK      NS_RGB(0, 0, 0)

#define MAX_BRIGHTNESS  254
#define MAX_DARKNESS     0
 
void NS_Get3DColors(nscolor aResult[2], nscolor aBackgroundColor)
{
  int rb = NS_GET_R(aBackgroundColor);
  int gb = NS_GET_G(aBackgroundColor);
  int bb = NS_GET_B(aBackgroundColor);
  
  int brightness = NS_GetBrightness(rb,gb,bb);

  int f0, f1;
  if (brightness < COLOR_DARK_THRESHOLD) {
    f0 = COLOR_DARK_BS_FACTOR;
    f1 = COLOR_DARK_TS_FACTOR;
  } else if (brightness > COLOR_LIGHT_THRESHOLD) {
    f0 = COLOR_LITE_BS_FACTOR;
    f1 = COLOR_LITE_TS_FACTOR;
  } else {
    f0 = COLOR_DARK_BS_FACTOR +
      (brightness *
       (COLOR_LITE_BS_FACTOR - COLOR_DARK_BS_FACTOR) / MAX_COLOR);
    f1 = COLOR_DARK_TS_FACTOR +
      (brightness *
       (COLOR_LITE_TS_FACTOR - COLOR_DARK_TS_FACTOR) / MAX_COLOR);
  }

  int r = rb - (f0 * rb / 100);
  int g = gb - (f0 * gb / 100);
  int b = bb - (f0 * bb / 100);
  aResult[0] = NS_RGB(r, g, b);
  if ((r == rb) && (g == gb) && (b == bb)) {
    aResult[0] = (aBackgroundColor == BLACK) ? DARK_GRAY : BLACK;
  }

  r = rb + (f1 * (MAX_COLOR - rb) / 100);
  if (r > 255) r = 255;
  g = gb + (f1 * (MAX_COLOR - gb) / 100);
  if (g > 255) g = 255;
  b = bb + (f1 * (MAX_COLOR - bb) / 100);
  if (b > 255) b = 255;
  aResult[1] = NS_RGB(r, g, b);
  if ((r == rb) && (g == gb) && (b == bb)) {
    aResult[1] = (aBackgroundColor == WHITE) ? LIGHT_GRAY : WHITE;
  }
}

void NS_GetSpecial3DColors(nscolor aResult[2],
											   nscolor aBackgroundColor,
											   nscolor aBorderColor)
{

  PRUint8 f0, f1;
  PRUint8 r, g, b;

  PRUint8 rb = NS_GET_R(aBorderColor);
  PRUint8 gb = NS_GET_G(aBorderColor);
  PRUint8 bb = NS_GET_B(aBorderColor);

  
  
  
  

  PRUint8 red = NS_GET_R(aBackgroundColor);
  PRUint8 green = NS_GET_G(aBackgroundColor);
  PRUint8 blue = NS_GET_B(aBackgroundColor);
  
  PRUint8 elementBrightness = NS_GetBrightness(rb,gb,bb);
  PRUint8 backgroundBrightness = NS_GetBrightness(red, green, blue);


  if (backgroundBrightness < COLOR_DARK_THRESHOLD) {
    f0 = COLOR_DARK_BS_FACTOR;
    f1 = COLOR_DARK_TS_FACTOR;
	if(elementBrightness == MAX_DARKNESS)
	{
       rb = NS_GET_R(DARK_GRAY);
       gb = NS_GET_G(DARK_GRAY);
       bb = NS_GET_B(DARK_GRAY);
	}
  }else if (backgroundBrightness > COLOR_LIGHT_THRESHOLD) {
    f0 = COLOR_LITE_BS_FACTOR;
    f1 = COLOR_LITE_TS_FACTOR;
	if(elementBrightness == MAX_BRIGHTNESS)
	{
       rb = NS_GET_R(LIGHT_GRAY);
       gb = NS_GET_G(LIGHT_GRAY);
       bb = NS_GET_B(LIGHT_GRAY);
	}
  }else {
    f0 = COLOR_DARK_BS_FACTOR +
      (backgroundBrightness *
       (COLOR_LITE_BS_FACTOR - COLOR_DARK_BS_FACTOR) / MAX_COLOR);
    f1 = COLOR_DARK_TS_FACTOR +
      (backgroundBrightness *
       (COLOR_LITE_TS_FACTOR - COLOR_DARK_TS_FACTOR) / MAX_COLOR);
  }
  
  
  r = rb - (f0 * rb / 100);
  g = gb - (f0 * gb / 100);
  b = bb - (f0 * bb / 100);
  aResult[0] = NS_RGB(r, g, b);

  r = rb + (f1 * (MAX_COLOR - rb) / 100);
  g = gb + (f1 * (MAX_COLOR - gb) / 100);
  b = bb + (f1 * (MAX_COLOR - bb) / 100);
  aResult[1] = NS_RGB(r, g, b);
}

int NS_GetBrightness(PRUint8 aRed, PRUint8 aGreen, PRUint8 aBlue)
{

  PRUint8 intensity = (aRed + aGreen + aBlue) / 3;

  PRUint8 luminosity = NS_GetLuminosity(NS_RGB(aRed, aGreen, aBlue)) / 1000;
 
  return ((intensity * INTENSITY_FACTOR) +
          (luminosity * LUMINOSITY_FACTOR)) / 100;
}

PRInt32 NS_GetLuminosity(nscolor aColor)
{
  return (NS_GET_R(aColor) * RED_LUMINOSITY +
          NS_GET_G(aColor) * GREEN_LUMINOSITY +
          NS_GET_B(aColor) * BLUE_LUMINOSITY);
}





void
NS_RGB2HSV(nscolor aColor,PRUint16 &aHue,PRUint16 &aSat,PRUint16 &aValue)
{
PRUint8  r,g,b;
PRInt16  delta,min,max,r1,b1,g1;
float    hue;

  r = NS_GET_R(aColor);
  g = NS_GET_G(aColor);
  b = NS_GET_B(aColor);

  if (r>g) {
    max = r;
    min = g;
  } else {
    max = g;
    min = r;
  }

  if (b>max) {
    max = b;
  }
  if (b<min) {
    min = b;
  }

  
  aValue = max;   
  delta = max-min;
  aSat = (max!=0)?((delta*255)/max):0;
  r1 = r;
  b1 = b;
  g1 = g;

  if (aSat==0) {
    hue = 1000;
  } else {
    if(r==max){
      hue=(float)(g1-b1)/(float)delta;
    } else if (g1==max) {
      hue= 2.0f+(float)(b1-r1)/(float)delta;
    } else { 
      hue = 4.0f+(float)(r1-g1)/(float)delta;
    }
  }

  if(hue<999) {
    hue*=60;
    if(hue<0){
      hue+=360;
    }
  } else {
    hue=0;
  }

  aHue = (PRUint16)hue;
}





void
NS_HSV2RGB(nscolor &aColor,PRUint16 aHue,PRUint16 aSat,PRUint16 aValue)
{
PRUint16  r=0,g=0,b=0;
PRUint16  i,p,q,t;
double    h,f,percent;

  if ( aSat == 0 ){
    
    r = aValue;
    g = aValue;
    b = aValue;
  } else {
    
    
    if (aHue >= 360) {
      aHue = 0;
    }

    
    
    
    h = (double)aHue / 60.0;
    i = (PRUint16) floor(h);
    f = h-(double)i;
    percent = ((double)aValue/255.0);   
                                        
    p = (PRUint16)(percent*(255-aSat));
    q = (PRUint16)(percent*(255-(aSat*f)));
    t = (PRUint16)(percent*(255-(aSat*(1.0-f))));

    
    switch(i){
      case 0: r = aValue; g = t; b = p;break;
      case 1: r = q; g = aValue; b = p;break;
      case 2: r = p; g = aValue; b = t;break;
      case 3: r = p; g = q; b = aValue;break;
      case 4: r = t; g = p; b = aValue;break;
      case 5: r = aValue; g = p; b = q;break;
    }
  }
  aColor = NS_RGB(r,g,b);
}
