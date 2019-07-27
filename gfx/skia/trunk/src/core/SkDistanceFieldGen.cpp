






#include "SkDistanceFieldGen.h"
#include "SkPoint.h"

struct DFData {
    float   fAlpha;      
    float   fDistSq;     
    SkPoint fDistVector; 
};

enum NeighborFlags {
    kLeft_NeighborFlag        = 0x01,
    kRight_NeighborFlag       = 0x02,
    kTopLeft_NeighborFlag     = 0x04,
    kTop_NeighborFlag         = 0x08,
    kTopRight_NeighborFlag    = 0x10,
    kBottomLeft_NeighborFlag  = 0x20,
    kBottom_NeighborFlag      = 0x40,
    kBottomRight_NeighborFlag = 0x80,
    kAll_NeighborFlags        = 0xff,

    kNeighborFlagCount        = 8
};





static bool found_edge(const unsigned char* imagePtr, int width, int neighborFlags) {
    
    const int kNum8ConnectedNeighbors = 8;
    const int offsets[8] = {-1, 1, -width-1, -width, -width+1, width-1, width, width+1 };
    SkASSERT(kNum8ConnectedNeighbors == kNeighborFlagCount);

    
    unsigned char currVal = *imagePtr;
    unsigned char currCheck = (currVal >> 7);
    for (int i = 0; i < kNum8ConnectedNeighbors; ++i) {
        unsigned char neighborVal;
        if ((1 << i) & neighborFlags) {
            const unsigned char* checkPtr = imagePtr + offsets[i];
            neighborVal = *checkPtr;
        } else {
            neighborVal = 0;
        }
        unsigned char neighborCheck = (neighborVal >> 7);
        SkASSERT(currCheck == 0 || currCheck == 1);
        SkASSERT(neighborCheck == 0 || neighborCheck == 1);
        
        if (currCheck != neighborCheck ||
            
            (!currCheck && !neighborCheck && currVal && neighborVal)) {
            return true;
        }
    }

    return false;
}

static void init_glyph_data(DFData* data, unsigned char* edges, const unsigned char* image,
                            int dataWidth, int dataHeight,
                            int imageWidth, int imageHeight,
                            int pad) {
    data += pad*dataWidth;
    data += pad;
    edges += (pad*dataWidth + pad);

    for (int j = 0; j < imageHeight; ++j) {
        for (int i = 0; i < imageWidth; ++i) {
            if (255 == *image) {
                data->fAlpha = 1.0f;
            } else {
                data->fAlpha = (*image)*0.00392156862f;  
            }
            int checkMask = kAll_NeighborFlags;
            if (i == 0) {
                checkMask &= ~(kLeft_NeighborFlag|kTopLeft_NeighborFlag|kBottomLeft_NeighborFlag);
            }
            if (i == imageWidth-1) {
                checkMask &= ~(kRight_NeighborFlag|kTopRight_NeighborFlag|kBottomRight_NeighborFlag);
            }
            if (j == 0) {
                checkMask &= ~(kTopLeft_NeighborFlag|kTop_NeighborFlag|kTopRight_NeighborFlag);
            }
            if (j == imageHeight-1) {
                checkMask &= ~(kBottomLeft_NeighborFlag|kBottom_NeighborFlag|kBottomRight_NeighborFlag);
            }
            if (found_edge(image, imageWidth, checkMask)) {
                *edges = 255;  
            }
            ++data;
            ++image;
            ++edges;
        }
        data += 2*pad;
        edges += 2*pad;
    }
}




static float edge_distance(const SkPoint& direction, float alpha) {
    float dx = direction.fX;
    float dy = direction.fY;
    float distance;
    if (SkScalarNearlyZero(dx) || SkScalarNearlyZero(dy)) {
        distance = 0.5f - alpha;
    } else {
        
        
        dx = SkScalarAbs(dx);
        dy = SkScalarAbs(dy);
        if (dx < dy) {
            SkTSwap(dx, dy);
        }

        
        
        float a1num = 0.5f*dy;

        
        

        
        if (alpha*dx < a1num) {
            
            distance = 0.5f*(dx + dy) - SkScalarSqrt(2.0f*dx*dy*alpha);
        
        } else if (alpha*dx < (dx - a1num)) {
            distance = (0.5f - alpha)*dx;
        
        } else {
            
            distance = -0.5f*(dx + dy) + SkScalarSqrt(2.0f*dx*dy*(1.0f - alpha));
        }
    }

    return distance;
}

static void init_distances(DFData* data, unsigned char* edges, int width, int height) {
    
    DFData* currData = data;
    DFData* prevData = data - width;
    DFData* nextData = data + width;

    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            if (*edges) {
                
                SkASSERT(i > 0 && i < width-1 && j > 0 && j < height-1);
                
                
                
                
                SkPoint currGrad;
                currGrad.fX = (prevData+1)->fAlpha - (prevData-1)->fAlpha
                             + SK_ScalarSqrt2*(currData+1)->fAlpha
                             - SK_ScalarSqrt2*(currData-1)->fAlpha
                             + (nextData+1)->fAlpha - (nextData-1)->fAlpha;
                currGrad.fY = (nextData-1)->fAlpha - (prevData-1)->fAlpha
                             + SK_ScalarSqrt2*nextData->fAlpha
                             - SK_ScalarSqrt2*prevData->fAlpha
                             + (nextData+1)->fAlpha - (prevData+1)->fAlpha;
                currGrad.setLengthFast(1.0f);

                
                float dist = edge_distance(currGrad, currData->fAlpha);
                currGrad.scale(dist, &currData->fDistVector);
                currData->fDistSq = dist*dist;
            } else {
                
                currData->fDistSq = 2000000.f;
                currData->fDistVector.fX = 1000.f;
                currData->fDistVector.fY = 1000.f;
            }
            ++currData;
            ++prevData;
            ++nextData;
            ++edges;
        }
    }
}





static void F1(DFData* curr, int width) {
    
    DFData* check = curr - width-1;
    SkPoint distVec = check->fDistVector;
    float distSq = check->fDistSq - 2.0f*(distVec.fX + distVec.fY - 1.0f);
    if (distSq < curr->fDistSq) {
        distVec.fX -= 1.0f;
        distVec.fY -= 1.0f;
        curr->fDistSq = distSq;
        curr->fDistVector = distVec;
    }

    
    check = curr - width;
    distVec = check->fDistVector;
    distSq = check->fDistSq - 2.0f*distVec.fY + 1.0f;
    if (distSq < curr->fDistSq) {
        distVec.fY -= 1.0f;
        curr->fDistSq = distSq;
        curr->fDistVector = distVec;
    }

    
    check = curr - width+1;
    distVec = check->fDistVector;
    distSq = check->fDistSq + 2.0f*(distVec.fX - distVec.fY + 1.0f);
    if (distSq < curr->fDistSq) {
        distVec.fX += 1.0f;
        distVec.fY -= 1.0f;
        curr->fDistSq = distSq;
        curr->fDistVector = distVec;
    }

    
    check = curr - 1;
    distVec = check->fDistVector;
    distSq = check->fDistSq - 2.0f*distVec.fX + 1.0f;
    if (distSq < curr->fDistSq) {
        distVec.fX -= 1.0f;
        curr->fDistSq = distSq;
        curr->fDistVector = distVec;
    }
}



static void F2(DFData* curr, int width) {
    
    DFData* check = curr + 1;
    float distSq = check->fDistSq;
    SkPoint distVec = check->fDistVector;
    distSq = check->fDistSq + 2.0f*distVec.fX + 1.0f;
    if (distSq < curr->fDistSq) {
        distVec.fX += 1.0f;
        curr->fDistSq = distSq;
        curr->fDistVector = distVec;
    }
}



static void B1(DFData* curr, int width) {
    
    DFData* check = curr - 1;
    SkPoint distVec = check->fDistVector;
    float distSq = check->fDistSq - 2.0f*distVec.fX + 1.0f;
    if (distSq < curr->fDistSq) {
        distVec.fX -= 1.0f;
        curr->fDistSq = distSq;
        curr->fDistVector = distVec;
    }
}



static void B2(DFData* curr, int width) {
    
    DFData* check = curr + 1;
    SkPoint distVec = check->fDistVector;
    float distSq = check->fDistSq + 2.0f*distVec.fX + 1.0f;
    if (distSq < curr->fDistSq) {
        distVec.fX += 1.0f;
        curr->fDistSq = distSq;
        curr->fDistVector = distVec;
    }

    
    check = curr + width-1;
    distVec = check->fDistVector;
    distSq = check->fDistSq - 2.0f*(distVec.fX - distVec.fY - 1.0f);
    if (distSq < curr->fDistSq) {
        distVec.fX -= 1.0f;
        distVec.fY += 1.0f;
        curr->fDistSq = distSq;
        curr->fDistVector = distVec;
    }

    
    check = curr + width;
    distVec = check->fDistVector;
    distSq = check->fDistSq + 2.0f*distVec.fY + 1.0f;
    if (distSq < curr->fDistSq) {
        distVec.fY += 1.0f;
        curr->fDistSq = distSq;
        curr->fDistVector = distVec;
    }

    
    check = curr + width+1;
    distVec = check->fDistVector;
    distSq = check->fDistSq + 2.0f*(distVec.fX + distVec.fY + 1.0f);
    if (distSq < curr->fDistSq) {
        distVec.fX += 1.0f;
        distVec.fY += 1.0f;
        curr->fDistSq = distSq;
        curr->fDistVector = distVec;
    }
}


#define DUMP_EDGE 0

#if !DUMP_EDGE
static unsigned char pack_distance_field_val(float dist, float distanceMagnitude) {
    if (dist <= -distanceMagnitude) {
        return 255;
    } else if (dist > distanceMagnitude) {
        return 0;
    } else {
        return (unsigned char)((distanceMagnitude-dist)*128.0f/distanceMagnitude);
    }
}
#endif



static bool generate_distance_field_from_image(unsigned char* distanceField,
                                               const unsigned char* copyPtr,
                                               int width, int height) {
    SkASSERT(NULL != distanceField);
    SkASSERT(NULL != copyPtr);

    
    
    int pad = SK_DistanceFieldPad + 1;

    
    int dataWidth = width + 2*pad;
    int dataHeight = height + 2*pad;

    
    size_t dataSize = dataWidth*dataHeight*sizeof(DFData);
    SkAutoSMalloc<1024> dfStorage(dataSize);
    DFData* dataPtr = (DFData*) dfStorage.get();
    sk_bzero(dataPtr, dataSize);

    SkAutoSMalloc<1024> edgeStorage(dataWidth*dataHeight*sizeof(char));
    unsigned char* edgePtr = (unsigned char*) edgeStorage.get();
    sk_bzero(edgePtr, dataWidth*dataHeight*sizeof(char));

    
    init_glyph_data(dataPtr, edgePtr, copyPtr,
                    dataWidth, dataHeight,
                    width+2, height+2, SK_DistanceFieldPad);

    
    init_distances(dataPtr, edgePtr, dataWidth, dataHeight);

    

    
    DFData* currData = dataPtr+dataWidth+1; 
    unsigned char* currEdge = edgePtr+dataWidth+1;
    for (int j = 1; j < dataHeight-1; ++j) {
        
        for (int i = 1; i < dataWidth-1; ++i) {
            
            if (!*currEdge) {
                F1(currData, dataWidth);
            }
            ++currData;
            ++currEdge;
        }

        
        --currData; 
        --currEdge;
        for (int i = 1; i < dataWidth-1; ++i) {
            
            if (!*currEdge) {
                F2(currData, dataWidth);
            }
            --currData;
            --currEdge;
        }

        currData += dataWidth+1;
        currEdge += dataWidth+1;
    }

    
    currData = dataPtr+dataWidth*(dataHeight-2) - 1; 
    currEdge = edgePtr+dataWidth*(dataHeight-2) - 1;
    for (int j = 1; j < dataHeight-1; ++j) {
        
        for (int i = 1; i < dataWidth-1; ++i) {
            
            if (!*currEdge) {
                B1(currData, dataWidth);
            }
            ++currData;
            ++currEdge;
        }

        
        --currData; 
        --currEdge;
        for (int i = 1; i < dataWidth-1; ++i) {
            
            if (!*currEdge) {
                B2(currData, dataWidth);
            }
            --currData;
            --currEdge;
        }

        currData -= dataWidth-1;
        currEdge -= dataWidth-1;
    }

    
    currData = dataPtr + dataWidth+1;
    currEdge = edgePtr + dataWidth+1;
    unsigned char *dfPtr = distanceField;
    for (int j = 1; j < dataHeight-1; ++j) {
        for (int i = 1; i < dataWidth-1; ++i) {
#if DUMP_EDGE
            float alpha = currData->fAlpha;
            float edge = 0.0f;
            if (*currEdge) {
                edge = 0.25f;
            }
            
            float result = alpha + (1.0f-alpha)*edge;
            unsigned char val = sk_float_round2int(255*result);
            *dfPtr++ = val;
#else
            float dist;
            if (currData->fAlpha > 0.5f) {
                dist = -SkScalarSqrt(currData->fDistSq);
            } else {
                dist = SkScalarSqrt(currData->fDistSq);
            }
            *dfPtr++ = pack_distance_field_val(dist, (float)SK_DistanceFieldMagnitude);
#endif
            ++currData;
            ++currEdge;
        }
        currData += 2;
        currEdge += 2;
    }

    return true;
}


bool SkGenerateDistanceFieldFromA8Image(unsigned char* distanceField,
                                        const unsigned char* image,
                                        int width, int height, int rowBytes) {
    SkASSERT(NULL != distanceField);
    SkASSERT(NULL != image);

    
    SkAutoSMalloc<1024> copyStorage((width+2)*(height+2)*sizeof(char));
    unsigned char* copyPtr = (unsigned char*) copyStorage.get();

    
    
    const unsigned char* currSrcScanLine = image;
    sk_bzero(copyPtr, (width+2)*sizeof(char));
    unsigned char* currDestPtr = copyPtr + width + 2;
    for (int i = 0; i < height; ++i) {
        *currDestPtr++ = 0;
        memcpy(currDestPtr, currSrcScanLine, rowBytes);
        currSrcScanLine += rowBytes;
        currDestPtr += width;
        *currDestPtr++ = 0;
    }
    sk_bzero(currDestPtr, (width+2)*sizeof(char));

    return generate_distance_field_from_image(distanceField, copyPtr, width, height);
}


bool SkGenerateDistanceFieldFromBWImage(unsigned char* distanceField,
                                        const unsigned char* image,
                                        int width, int height, int rowBytes) {
    SkASSERT(NULL != distanceField);
    SkASSERT(NULL != image);

    
    SkAutoSMalloc<1024> copyStorage((width+2)*(height+2)*sizeof(char));
    unsigned char* copyPtr = (unsigned char*) copyStorage.get();

    
    
    const unsigned char* currSrcScanLine = image;
    sk_bzero(copyPtr, (width+2)*sizeof(char));
    unsigned char* currDestPtr = copyPtr + width + 2;
    for (int i = 0; i < height; ++i) {
        *currDestPtr++ = 0;
        int rowWritesLeft = width;
        const unsigned char *maskPtr = currSrcScanLine;
        while (rowWritesLeft > 0) {
            unsigned mask = *maskPtr++;
            for (int i = 7; i >= 0 && rowWritesLeft; --i, --rowWritesLeft) {
                *currDestPtr++ = (mask & (1 << i)) ? 0xff : 0;
            }
        }
        currSrcScanLine += rowBytes;
        *currDestPtr++ = 0;
    }
    sk_bzero(currDestPtr, (width+2)*sizeof(char));

    return generate_distance_field_from_image(distanceField, copyPtr, width, height);
}
