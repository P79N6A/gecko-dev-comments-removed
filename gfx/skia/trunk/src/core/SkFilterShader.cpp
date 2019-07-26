






#include "SkFilterShader.h"

#include "SkColorFilter.h"
#include "SkReadBuffer.h"
#include "SkWriteBuffer.h"
#include "SkShader.h"
#include "SkString.h"

SkFilterShader::SkFilterShader(SkShader* shader, SkColorFilter* filter) {
    fShader = shader;
    shader->ref();

    fFilter = filter;
    filter->ref();
}

SkFilterShader::SkFilterShader(SkReadBuffer& buffer)
    : INHERITED(buffer) {
    fShader = buffer.readShader();
    fFilter = buffer.readColorFilter();
}

SkFilterShader::~SkFilterShader() {
    fFilter->unref();
    fShader->unref();
}

void SkFilterShader::flatten(SkWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    buffer.writeFlattenable(fShader);
    buffer.writeFlattenable(fFilter);
}

uint32_t SkFilterShader::getFlags() {
    uint32_t shaderF = fShader->getFlags();
    uint32_t filterF = fFilter->getFlags();

    
    if (!(filterF & SkColorFilter::kHasFilter16_Flag)) {
        shaderF &= ~SkShader::kHasSpan16_Flag;
    }
    
    if (!(filterF & SkColorFilter::kAlphaUnchanged_Flag)) {
        shaderF &= ~(SkShader::kOpaqueAlpha_Flag | SkShader::kHasSpan16_Flag);
    }
    return shaderF;
}

bool SkFilterShader::setContext(const SkBitmap& device,
                                const SkPaint& paint,
                                const SkMatrix& matrix) {
    
    

    if (!this->INHERITED::setContext(device, paint, matrix)) {
        return false;
    }
    if (!fShader->setContext(device, paint, matrix)) {
        this->INHERITED::endContext();
        return false;
    }
    return true;
}

void SkFilterShader::endContext() {
    fShader->endContext();
    this->INHERITED::endContext();
}

void SkFilterShader::shadeSpan(int x, int y, SkPMColor result[], int count) {
    fShader->shadeSpan(x, y, result, count);
    fFilter->filterSpan(result, count, result);
}

void SkFilterShader::shadeSpan16(int x, int y, uint16_t result[], int count) {
    SkASSERT(fShader->getFlags() & SkShader::kHasSpan16_Flag);
    SkASSERT(fFilter->getFlags() & SkColorFilter::kHasFilter16_Flag);

    fShader->shadeSpan16(x, y, result, count);
    fFilter->filterSpan16(result, count, result);
}

#ifndef SK_IGNORE_TO_STRING
void SkFilterShader::toString(SkString* str) const {
    str->append("SkFilterShader: (");

    str->append("Shader: ");
    fShader->toString(str);
    str->append(" Filter: ");
    

    this->INHERITED::toString(str);

    str->append(")");
}
#endif
