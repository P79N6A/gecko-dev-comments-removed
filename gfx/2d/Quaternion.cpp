




#include "Quaternion.h"
#include "Matrix.h"
#include "Tools.h"
#include <algorithm>
#include <ostream>
#include <math.h>

using namespace std;

namespace mozilla {
namespace gfx {

std::ostream&
operator<<(std::ostream& aStream, const Quaternion& aQuat)
{
  return aStream << "< " << aQuat.x << " "  << aQuat.y << " " << aQuat.z << " " << aQuat.w << ">";
}

void
Quaternion::SetFromRotationMatrix(const Matrix4x4& m)
{
  
  const Float trace = m._11 + m._22 + m._33;
  if (trace > 0.0) {
    const Float s = 0.5f / sqrt(trace + 1.0f);
    w = 0.25f / s;
    x = (m._32 - m._23) * s;
    y = (m._13 - m._31) * s;
    z = (m._21 - m._12) * s;
  } else if (m._11 > m._22 && m._11 > m._33) {
    const Float s = 2.0f * sqrt(1.0f + m._11 - m._22 - m._33);
    w = (m._32 - m._23) / s;
    x = 0.25f * s;
    y = (m._12 + m._21) / s;
    z = (m._13 + m._31) / s;
  } else if (m._22 > m._33) {
    const Float s = 2.0 * sqrt(1.0f + m._22 - m._11 - m._33);
    w = (m._13 - m._31) / s;
    x = (m._12 + m._21) / s;
    y = 0.25f * s;
    z = (m._23 + m._32) / s;
  } else {
    const Float s = 2.0 * sqrt(1.0f + m._33 - m._11 - m._22);
    w = (m._21 - m._12) / s;
    x = (m._13 + m._31) / s;
    y = (m._23 + m._32) / s;
    z = 0.25f * s;
  }
}

} 
} 
