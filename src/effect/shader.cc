#include <skity/effect/shader.hpp>

#include "src/effect/gradient_shader.hpp"

namespace skity {

Shader::GradientType Shader::asGradient(GradientInfo *info) const {
  return GradientType::kNone;
}

std::shared_ptr<Shader> Shader::MakeLinear(const Point pts[2],
                                           const Vec4 colors[],
                                           const float pos[], int count) {
  // count must be >= 2
  if (count <= 2) {
    return nullptr;
  }

  return std::make_shared<LinearGradientShader>(pts, colors, pos, count);
}

}  // namespace skity