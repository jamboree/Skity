#include "src/render/gl/gl_draw_op.hpp"

#include "src/render/gl/draw/gl_stencil_op.hpp"
#include "src/render/gl/gl_shader.hpp"

namespace skity {

void GLDrawOp::Draw() {
  OnBeforeDraw();
  OnDraw();
  OnAfterDraw();
}

void GLDrawOp::Init() { OnInit(); }

StencilShader* GLDrawOpBuilder::stencil_shader = nullptr;
GLMesh* GLDrawOpBuilder::gl_mesh = nullptr;
uint32_t GLDrawOpBuilder::front_start = 0;
uint32_t GLDrawOpBuilder::front_count = 0;
uint32_t GLDrawOpBuilder::back_start = 0;
uint32_t GLDrawOpBuilder::back_count = 0;

void GLDrawOpBuilder::UpdateStencilShader(StencilShader* shader) {
  stencil_shader = shader;
}

void GLDrawOpBuilder::UpdateMesh(GLMesh* mesh) { gl_mesh = mesh; }

void GLDrawOpBuilder::UpdateFrontStart(uint32_t value) { front_start = value; }

void GLDrawOpBuilder::UpdateFrontCount(uint32_t value) { front_count = value; }

void GLDrawOpBuilder::UpdateBackStart(uint32_t value) { back_start = value; }

void GLDrawOpBuilder::UpdateBackCount(uint32_t value) { back_count = value; }

std::unique_ptr<GLDrawOp> GLDrawOpBuilder::CreateStencilOp(float stroke_width,
                                                           bool positive) {
  //
  auto op =
      std::make_unique<GLStencilDrawOp>(front_start, front_count, back_start,
                                        back_count, stencil_shader, gl_mesh);

  return op;
}

}  // namespace skity
