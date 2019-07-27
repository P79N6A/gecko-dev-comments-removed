



"use strict";

importScripts("ClusterLib.js", "ColorConversion.js");


const PIXEL_RED = 0;
const PIXEL_GREEN = 1;
const PIXEL_BLUE = 2;
const PIXEL_ALPHA = 3;


const NUM_COMPONENTS = 4;


const RED_SHIFT = 16;
const GREEN_SHIFT = 8;



const MAX_COLORS_TO_MERGE = 500;




const MERGE_THRESHOLD = 12;


const MAX_SIZE_HANDICAP = 5;

const SIZE_HANDICAP_CUTOFF = 2;



const BACKGROUND_THRESHOLD = 10;



const MIN_ALPHA = 25;



const BACKGROUND_WEIGHT_THRESHOLD = 15;




const CHROMA_WEIGHT_UPPER = 90;
const CHROMA_WEIGHT_LOWER = 1;
const CHROMA_WEIGHT_MIDDLE = (CHROMA_WEIGHT_UPPER + CHROMA_WEIGHT_LOWER) / 2;














onmessage = function(event) {
  let imageData = event.data.imageData;
  let pixels = imageData.data;
  let width = imageData.width;
  let height = imageData.height;
  let maxColors = event.data.maxColors;
  if (typeof(maxColors) != "number") {
    maxColors = 1;
  }

  let allColors = getColors(pixels, width, height);

  
  let mergedColors = mergeColors(allColors.slice(0, MAX_COLORS_TO_MERGE),
                                 width * height, MERGE_THRESHOLD);

  let backgroundColor = getBackgroundColor(pixels, width, height);

  mergedColors = mergedColors.map(function(cluster) {
    
    
    let metadata = cluster.item;

    
    
    
    metadata.desirability = metadata.ratio;
    let weight = 1;

    
    if (backgroundColor != null) {
      let backgroundDistance = labEuclidean(metadata.mean, backgroundColor);
      if (backgroundDistance < BACKGROUND_WEIGHT_THRESHOLD) {
        weight = backgroundDistance / BACKGROUND_WEIGHT_THRESHOLD;
      }
    }

    
    
    
    let chroma = labChroma(metadata.mean);
    if (chroma < CHROMA_WEIGHT_LOWER) {
      chroma = CHROMA_WEIGHT_LOWER;
    } else if (chroma > CHROMA_WEIGHT_UPPER) {
      chroma = CHROMA_WEIGHT_UPPER;
    }
    weight *= chroma / CHROMA_WEIGHT_MIDDLE;

    metadata.desirability *= weight;
    return metadata;
  });

  
  mergedColors.sort(function(a, b) {
    return b.desirability != a.desirability ? b.desirability - a.desirability : b.color - a.color;
  });
  mergedColors = mergedColors.map(function(metadata) {
    return metadata.color;
  }).slice(0, maxColors);
  postMessage({ colors: mergedColors });
};
















function getColors(pixels, width, height) {
  let colorFrequency = {};
  for (let x = 0; x < width; x++) {
    for (let y = 0; y < height; y++) {
      let offset = (x * NUM_COMPONENTS) + (y * NUM_COMPONENTS * width);

      if (pixels[offset + PIXEL_ALPHA] < MIN_ALPHA) {
        continue;
      }

      let color = pixels[offset + PIXEL_RED] << RED_SHIFT
                | pixels[offset + PIXEL_GREEN] << GREEN_SHIFT
                | pixels[offset + PIXEL_BLUE];

      if (color in colorFrequency) {
        colorFrequency[color]++;
      } else {
        colorFrequency[color] = 1;
      }
    }
  }

  let colors = [];
  for (var color in colorFrequency) {
    colors.push({ color: +color, freq: colorFrequency[+color] });
  }
  colors.sort(descendingFreqSort);
  return colors;
}



















function mergeColors(colorFrequencies, numPixels, threshold) {
  let items = colorFrequencies.map(function(colorFrequency) {
    let color = colorFrequency.color;
    let freq = colorFrequency.freq;
    return {
      mean: rgb2lab(color >> RED_SHIFT, color >> GREEN_SHIFT & 0xff,
                    color & 0xff),
      
      
      color: color,
      colors: [color],
      highFreq: freq,
      highRatio: freq / numPixels,
      
      highColor: color,
      
      ratio: freq / numPixels,
      freq: freq,
    };
  });

  let merged = clusterlib.hcluster(items, distance, merge, threshold);
  return merged;
}

function descendingFreqSort(a, b) {
  return b.freq != a.freq ? b.freq - a.freq : b.color - a.color;
}














function distance(item1, item2) {
  
  let minRatio = Math.min(item1.ratio, item2.ratio);
  let dist = labEuclidean(item1.mean, item2.mean);
  let handicap = Math.min(MAX_SIZE_HANDICAP, dist * minRatio);
  if (handicap <= SIZE_HANDICAP_CUTOFF) {
    handicap = 0;
  }
  return dist + handicap;
}











function labEuclidean(color1, color2) {
  return Math.sqrt(
      Math.pow(color2.lightness - color1.lightness, 2)
    + Math.pow(color2.a - color1.a, 2)
    + Math.pow(color2.b - color1.b, 2));
}














function merge(item1, item2) {
  let lab1 = item1.mean;
  let lab2 = item2.mean;

  
  let num1 = item1.freq;
  let num2 = item2.freq;

  let total = num1 + num2;

  let mean = {
    lightness: (lab1.lightness * num1 + lab2.lightness * num2) / total,
    a: (lab1.a * num1 + lab2.a * num2) / total,
    b: (lab1.b * num1 + lab2.b * num2) / total
  };

  let colors = item1.colors.concat(item2.colors);

  
  let color;
  let avgFreq = colors.length / (item1.freq + item2.freq);
  if ((item1.highFreq > item2.highFreq) && (item1.highFreq > avgFreq * 2)) {
    color = item1.highColor;
  } else if (item2.highFreq > avgFreq * 2) {
    color = item2.highColor;
  } else {
    
    let minDist = Infinity, closest = 0;
    for (let i = 0; i < colors.length; i++) {
      let color = colors[i];
      let lab = rgb2lab(color >> RED_SHIFT, color >> GREEN_SHIFT & 0xff,
                        color & 0xff);
      let dist = labEuclidean(lab, mean);
      if (dist < minDist) {
        minDist = dist;
        closest = i;
      }
    }
    color = colors[closest];
  }

  const higherItem = item1.highFreq > item2.highFreq ? item1 : item2;

  return {
    mean: mean,
    color: color,
    highFreq: higherItem.highFreq,
    highColor: higherItem.highColor,
    highRatio: higherItem.highRatio,
    ratio: item1.ratio + item2.ratio,
    freq: item1.freq + item2.freq,
    colors: colors,
  };
}














function getBackgroundColor(pixels, width, height) {
  
  
  let coordinates = [[0, 0], [width - 1, 0], [width - 1, height - 1],
                     [0, height - 1]];

  
  let cornerColors = [];
  for (let i = 0; i < coordinates.length; i++) {
    let offset = (coordinates[i][0] * NUM_COMPONENTS)
               + (coordinates[i][1] * NUM_COMPONENTS * width);
    if (pixels[offset + PIXEL_ALPHA] < MIN_ALPHA) {
      
      continue;
    }
    cornerColors.push(rgb2lab(pixels[offset + PIXEL_RED],
                              pixels[offset + PIXEL_GREEN],
                              pixels[offset + PIXEL_BLUE]));
  }

  
  if (cornerColors.length <= 1) {
    return null;
  }

  
  let averageColor = { lightness: 0, a: 0, b: 0 };
  cornerColors.forEach(function(color) {
    for (let i in color) {
      averageColor[i] += color[i];
    }
  });
  for (let i in averageColor) {
    averageColor[i] /= cornerColors.length;
  }

  
  let threshold = BACKGROUND_THRESHOLD
                * (cornerColors.length / coordinates.length);

  
  
  for (let cornerColor of cornerColors) {
    if (labEuclidean(cornerColor, averageColor) > threshold) {
      return null;
    }
  }
  return averageColor;
}
