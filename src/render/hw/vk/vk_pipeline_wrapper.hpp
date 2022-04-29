#ifndef SKITY_SRC_RENDER_HW_VK_VK_PIPELINE_WRAPPER_HPP
#define SKITY_SRC_RENDER_HW_VK_VK_PIPELINE_WRAPPER_HPP

#include <vulkan/vulkan.h>

#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <skity/gpu/gpu_vk_context.hpp>
#include <tuple>
#include <vector>

#include "src/render/hw/hw_renderer.hpp"
#include "src/render/hw/vk/vk_interface.hpp"
#include "src/render/hw/vk/vk_utils.hpp"

#define SAFE_DESTROY(pipeline, ctx) \
  do {                              \
    if (pipeline) {                 \
      pipeline->Destroy(ctx);       \
    }                               \
  } while (false)

namespace skity {

class VKMemoryAllocator;
class SKVkFrameBufferData;
class VKTexture;

struct GlobalPushConst {
  alignas(16) glm::mat4 mvp = {};
  alignas(16) int32_t premul_alpha = {};
};

struct CommonFragmentSet {
  // [global_alpha, stroke_width, TBD, TBD]
  alignas(16) glm::vec4 info = {};
};

struct ColorInfoSet {
  alignas(16) glm::vec4 user_color = {};
};

struct GradientInfo {
  enum {
    MAX_COLORS = 32,
  };

  alignas(16) glm::ivec4 count = {};
  alignas(16) glm::vec4 bounds = {};
  alignas(16) glm::vec4 colors[MAX_COLORS];
  alignas(16) glm::vec4 pos[MAX_COLORS / 4];
};

struct ComputeInfo {
  // [radius, TBD, TBD, TBD]
  alignas(16) glm::vec4 info = {};
  // [BlurType, buffer_width, buffer_height, TBD]
  alignas(16) glm::ivec4 blur_type = {};
  // [ left, top, right, bottom ]
  alignas(16) glm::vec4 bounds = {};
};

class AbsPipelineWrapper : public VkInterfaceClient {
 public:
  AbsPipelineWrapper(bool use_gs) : use_gs_(use_gs) {}
  virtual ~AbsPipelineWrapper() = default;

  virtual bool IsComputePipeline() = 0;

  virtual bool HasColorSet() = 0;

  virtual void Init(GPUVkContext* ctx, VkShaderModule vertex,
                    VkShaderModule fragment, VkShaderModule geometry) = 0;

  virtual void Destroy(GPUVkContext* ctx) = 0;

  virtual void Bind(VkCommandBuffer cmd) = 0;

  virtual void Dispatch(VkCommandBuffer cmd, GPUVkContext* ctx) {}

  virtual VkDescriptorSetLayout GetFontSetLayout() { return VK_NULL_HANDLE; }

  virtual void UpdateStencilInfo(uint32_t reference, uint32_t compare_mask,
                                 uint32_t write_mask, GPUVkContext* ctx) {}

  virtual void UploadFontSet(VkDescriptorSet set, GPUVkContext* ctx) {}

  virtual void UploadPushConstant(GlobalPushConst const& push_const,
                                  VkCommandBuffer cmd) {}

  virtual void UploadTransformMatrix(glm::mat4 const& matrix, GPUVkContext* ctx,
                                     SKVkFrameBufferData* frame_buffer,
                                     VKMemoryAllocator* allocator) {}

  virtual void UploadCommonSet(CommonFragmentSet const& common_set,
                               GPUVkContext* ctx,
                               SKVkFrameBufferData* frame_buffer,
                               VKMemoryAllocator* allocator) {}

  virtual void UploadUniformColor(ColorInfoSet const& info, GPUVkContext* ctx,
                                  SKVkFrameBufferData* frame_buffer,
                                  VKMemoryAllocator* allocator) {}

  virtual void UploadGradientInfo(GradientInfo const& info, GPUVkContext* ctx,
                                  SKVkFrameBufferData* frame_buffer,
                                  VKMemoryAllocator* allocator) {}

  virtual void UploadImageTexture(VKTexture* texture, GPUVkContext* ctx,
                                  SKVkFrameBufferData* frame_buffer,
                                  VKMemoryAllocator* allocator) {}

  virtual void UploadBlurInfo(glm::ivec4 const& info, GPUVkContext* ctx,
                              SKVkFrameBufferData* frame_buffer,
                              VKMemoryAllocator* allocator) {}

  static std::unique_ptr<AbsPipelineWrapper> CreateStaticBlurPipeline(
      VKInterface* vk_interface, GPUVkContext* ctx, bool use_gs);

  static std::unique_ptr<AbsPipelineWrapper> CreateStaticBlurPipeline(
      VKInterface* vk_interface, GPUVkContext* ctx, bool use_gs,
      VkRenderPass render_pass);

  static std::unique_ptr<AbsPipelineWrapper> CreateComputeBlurPipeline(
      VKInterface* vk_interface, GPUVkContext* ctx);

 protected:
  bool UseGeometryShader() const { return use_gs_; }

 private:
  bool use_gs_;
};

class RenderPipeline : public AbsPipelineWrapper {
 public:
  RenderPipeline(bool use_gs, size_t push_const_size)
      : AbsPipelineWrapper(use_gs), push_const_size_(push_const_size) {}
  ~RenderPipeline() override = default;

  void SetRenderPass(VkRenderPass render_pass) {
    os_render_pass_ = render_pass;
  }

  bool IsComputePipeline() override { return false; }

  void Init(GPUVkContext* ctx, VkShaderModule vertex, VkShaderModule fragment,
            VkShaderModule geometry) override;

  void Destroy(GPUVkContext* ctx) override;

  void Bind(VkCommandBuffer cmd) override;

  void UploadPushConstant(GlobalPushConst const& push_const,
                          VkCommandBuffer cmd) override;

  void UploadTransformMatrix(glm::mat4 const& matrix, GPUVkContext* ctx,
                             SKVkFrameBufferData* frame_buffer,
                             VKMemoryAllocator* allocator) override;

  void UploadCommonSet(CommonFragmentSet const& common_set, GPUVkContext* ctx,
                       SKVkFrameBufferData* frame_buffer,
                       VKMemoryAllocator* allocator) override;

  void UploadFontSet(VkDescriptorSet set, GPUVkContext* ctx) override;

  bool HasColorSet() override {
    return descriptor_set_layout_[2] != VK_NULL_HANDLE;
  }

  VkDescriptorSetLayout GetFontSetLayout() override {
    return descriptor_set_layout_[3];
  }

 protected:
  void InitDescriptorSetLayout(GPUVkContext* ctx);
  void InitPipelineLayout(GPUVkContext* ctx);

  virtual VkDescriptorSetLayout GenerateColorSetLayout(GPUVkContext* ctx) = 0;

  virtual VkPipelineDepthStencilStateCreateInfo
  GetDepthStencilStateCreateInfo() = 0;

  virtual VkPipelineColorBlendAttachmentState GetColorBlendState();

  virtual std::vector<VkDynamicState> GetDynamicStates();

  VkDescriptorSetLayout GetColorSetLayout() {
    return descriptor_set_layout_[2];
  }

  VkPipelineLayout GetPipelineLayout() { return pipeline_layout_; }

  VkCommandBuffer GetBindCMD() { return bind_cmd_; }

  static VkPipelineDepthStencilStateCreateInfo StencilDiscardInfo();
  static VkPipelineDepthStencilStateCreateInfo StencilLessDiscardInfo();
  static VkPipelineDepthStencilStateCreateInfo StencilKeepInfo();

 private:
  VkVertexInputBindingDescription GetVertexInputBinding();
  std::array<VkVertexInputAttributeDescription, 2> GetVertexInputAttributes();

 private:
  size_t push_const_size_;
  VkRenderPass os_render_pass_ = VK_NULL_HANDLE;
  /**
   * set 0: common transform matrix info
   * set 1: common fragment color info, [global alpha, stroke width]
   * set 2: fragment color info
   * set 3: fragment font info
   */
  std::array<VkDescriptorSetLayout, 4> descriptor_set_layout_ = {};
  VkPipelineLayout pipeline_layout_ = {};
  VkPipeline pipeline_ = {};
  VkCommandBuffer bind_cmd_ = {};
};

class ComputePipeline : public AbsPipelineWrapper {
  enum {
    LOCAL_SIZE = 16,
  };

 public:
  ComputePipeline() : AbsPipelineWrapper(false) {}
  ~ComputePipeline() override = default;

  void Init(GPUVkContext* ctx, VkShaderModule vertex, VkShaderModule fragment,
            VkShaderModule geometry) override;

  void Destroy(GPUVkContext* ctx) override;

  void Bind(VkCommandBuffer cmd) override;

  void Dispatch(VkCommandBuffer cmd, GPUVkContext* ctx) override;

  void UploadCommonSet(CommonFragmentSet const& common_set, GPUVkContext* ctx,
                       SKVkFrameBufferData* frame_buffer,
                       VKMemoryAllocator* allocator) override;

  void UploadGradientInfo(GradientInfo const& info, GPUVkContext* ctx,
                          SKVkFrameBufferData* frame_buffer,
                          VKMemoryAllocator* allocator) override;

  void UploadImageTexture(VKTexture* texture, GPUVkContext* ctx,
                          SKVkFrameBufferData* frame_buffer,
                          VKMemoryAllocator* allocator) override;

  bool IsComputePipeline() override { return true; }

  bool HasColorSet() override { return false; }

  void UploadOutputTexture(VKTexture* texture);

 protected:
  glm::vec4 const& CommonInfo() const { return common_info_; }
  glm::vec4 const& BoundsInfo() const { return bounds_info_; }

  VKTexture* InputTexture() const { return input_texture_; }
  VKTexture* OutpuTexture() const { return output_texture_; }

  SKVkFrameBufferData* FrameBufferData() const { return frame_buffer_data_; }
  VKMemoryAllocator* Allocator() const { return allocator_; }

  VkPipelineLayout GetPipelineLayout() { return pipeline_layout_; }

  virtual VkDescriptorSetLayout CreateDescriptorSetLayout(
      GPUVkContext* ctx) = 0;

  virtual void OnDispatch(VkCommandBuffer cmd, GPUVkContext* ctx) = 0;

  VkDescriptorSetLayout ComputeSetLayout() const {
    return descriptor_set_layout_;
  }

 private:
  void UpdateFrameDataAndAllocator(SKVkFrameBufferData* frame_data,
                                   VKMemoryAllocator* allocator) {
    frame_buffer_data_ = frame_data;
    allocator_ = allocator;
  }

  void InitPipelineLayout(GPUVkContext* ctx);

 private:
  glm::vec4 common_info_ = {};
  glm::vec4 bounds_info_ = {};
  VKTexture* input_texture_ = {};
  VKTexture* output_texture_ = {};

  SKVkFrameBufferData* frame_buffer_data_ = {};
  VKMemoryAllocator* allocator_ = {};

  VkCommandBuffer bind_cmd_ = {};
  VkPipelineLayout pipeline_layout_ = {};
  VkDescriptorSetLayout descriptor_set_layout_ = {};
  VkPipeline pipeline_ = {};
};

class PipelineFamily : public VkInterfaceClient {
 public:
  virtual ~PipelineFamily() = default;

  void Init(GPUVkContext* ctx, bool use_geometry_shader,
            VkRenderPass os_renderpass = VK_NULL_HANDLE) {
    use_geometry_shader_ = use_geometry_shader;
    os_render_pass_ = os_renderpass;

    OnInit(ctx);
  }

  void Destroy(GPUVkContext* ctx) { OnDestroy(ctx); }

  void UpdateStencilFunc(HWStencilFunc func, HWStencilOp op) {
    stencil_func_ = func;
    stencil_op_ = op;
  }

  void UpdateStencilMask(uint8_t write_mask, uint8_t compare_mask) {
    stencil_write_mask_ = write_mask;
    stencil_compare_mask_ = compare_mask;
  }

  virtual AbsPipelineWrapper* ChoosePipeline(bool enable_stencil,
                                             bool off_screen) = 0;

  static std::unique_ptr<PipelineFamily> CreateColorPipelineFamily();
  static std::unique_ptr<PipelineFamily> CreateGradientPipelineFamily();
  static std::unique_ptr<PipelineFamily> CreateImagePipelineFamily();
  static std::unique_ptr<PipelineFamily> CreateStencilPipelineFamily();

  template <class T>
  std::unique_ptr<T> CreatePipeline(GPUVkContext* ctx, VkShaderModule vs_shader,
                                    VkShaderModule fs_shader,
                                    VkShaderModule gs_shader,
                                    VkRenderPass render_pass = VK_NULL_HANDLE) {
    auto pipeline =
        std::make_unique<T>(UseGeometryShader(), sizeof(GlobalPushConst));
    pipeline->SetInterface(GetInterface());
    pipeline->SetRenderPass(render_pass);

    pipeline->Init(ctx, vs_shader, fs_shader, gs_shader);

    return pipeline;
  }

 protected:
  VkRenderPass OffScreenRenderPass() const { return os_render_pass_; }

  bool UseGeometryShader() const { return use_geometry_shader_; }

  HWStencilFunc StencilFunc() const { return stencil_func_; }

  HWStencilOp StencilOp() const { return stencil_op_; }

  uint8_t WriteMask() const { return stencil_write_mask_; }
  uint8_t CompareMask() const { return stencil_compare_mask_; }

  virtual void OnInit(GPUVkContext* ctx) = 0;
  virtual void OnDestroy(GPUVkContext* ctx) = 0;

 private:
  bool use_geometry_shader_ = false;
  VkRenderPass os_render_pass_ = VK_NULL_HANDLE;
  HWStencilFunc stencil_func_ = HWStencilFunc::ALWAYS;
  HWStencilOp stencil_op_ = HWStencilOp::KEEP;
  uint8_t stencil_write_mask_ = 0xFF;
  uint8_t stencil_compare_mask_ = 0xFF;
};

class RenderPipelineFamily : public PipelineFamily {
 public:
  RenderPipelineFamily() = default;

  virtual ~RenderPipelineFamily() = default;

  AbsPipelineWrapper* ChoosePipeline(bool enable_stencil,
                                     bool off_screen) override;

 protected:
  void OnInit(GPUVkContext* ctx) override;
  void OnDestroy(GPUVkContext* ctx) override;

  virtual std::tuple<const char*, size_t> GetVertexShaderInfo();
  virtual std::tuple<const char*, size_t> GetFragmentShaderInfo() = 0;

  virtual std::unique_ptr<AbsPipelineWrapper> CreateStaticPipeline(
      GPUVkContext* ctx) = 0;
  virtual std::unique_ptr<AbsPipelineWrapper> CreateStencilDiscardPipeline(
      GPUVkContext* ctx) = 0;
  virtual std::unique_ptr<AbsPipelineWrapper> CreateStencilClipPipeline(
      GPUVkContext* ctx) = 0;
  virtual std::unique_ptr<AbsPipelineWrapper> CreateStencilKeepPipeline(
      GPUVkContext* ctx) = 0;
  virtual std::unique_ptr<AbsPipelineWrapper> CreateOSStaticPipeline(
      GPUVkContext* ctx) = 0;
  virtual std::unique_ptr<AbsPipelineWrapper> CreateOSStencilPipeline(
      GPUVkContext* ctx) = 0;

  VkShaderModule GetVertexShader() { return vs_shader_; }
  VkShaderModule GetFragmentShader() { return fs_shader_; }
  VkShaderModule GetGeometryShader() { return gs_shader_; }

 private:
  AbsPipelineWrapper* ChooseOffScreenPiepline(bool enable_stencil);
  AbsPipelineWrapper* ChooseRenderPipeline(bool enable_stencil);

  VkShaderModule GenerateVertexShader(GPUVkContext* ctx);
  VkShaderModule GenerateFragmentShader(GPUVkContext* ctx);
  VkShaderModule GenerateGeometryShader(GPUVkContext* ctx);

 private:
  VkShaderModule vs_shader_ = VK_NULL_HANDLE;
  VkShaderModule fs_shader_ = VK_NULL_HANDLE;
  VkShaderModule gs_shader_ = VK_NULL_HANDLE;
  std::unique_ptr<AbsPipelineWrapper> static_pipeline_ = {};
  std::unique_ptr<AbsPipelineWrapper> stencil_discard_pipeline_ = {};
  std::unique_ptr<AbsPipelineWrapper> stencil_clip_pipeline_ = {};
  std::unique_ptr<AbsPipelineWrapper> stencil_keep_pipeline_ = {};
  std::unique_ptr<AbsPipelineWrapper> os_static_pipeline_ = {};
  std::unique_ptr<AbsPipelineWrapper> os_stencil_pipeline_ = {};
};

template <class T>
struct PipelineBuilder {
  VKInterface* vk_interface;
  const char* vertex_src;
  size_t vertex_size;
  const char* fragment_src;
  size_t fragment_size;
  const char* geometry_src;
  size_t geometry_size;
  GPUVkContext* ctx;
  VkRenderPass render_pass;

  PipelineBuilder(VKInterface* interface, const char* vertex_src,
                  size_t vertex_size, const char* fragment_src,
                  size_t fragment_size, const char* geometry_src,
                  size_t geometry_size, GPUVkContext* ctx,
                  VkRenderPass r = VK_NULL_HANDLE)
      : vk_interface(interface),
        vertex_src(vertex_src),
        vertex_size(vertex_size),
        fragment_src(fragment_src),
        fragment_size(fragment_size),
        geometry_src(geometry_src),
        geometry_size(geometry_size),
        ctx(ctx),
        render_pass(r) {}

  std::unique_ptr<T> operator()() {
    auto pipeline =
        std::make_unique<T>(geometry_size > 0, sizeof(GlobalPushConst));

    if (render_pass) {
      pipeline->SetRenderPass(render_pass);
    }

    auto vertex = VKUtils::CreateShader(vk_interface, ctx->GetDevice(),
                                        (const char*)vertex_src, vertex_size);

    auto fragment =
        VKUtils::CreateShader(vk_interface, ctx->GetDevice(),
                              (const char*)fragment_src, fragment_size);

    VkShaderModule geometry = VK_NULL_HANDLE;
    if (geometry_size > 0) {
      geometry =
          VKUtils::CreateShader(vk_interface, ctx->GetDevice(),
                                (const char*)geometry_src, geometry_size);
    }
    pipeline->SetInterface(vk_interface);
    pipeline->Init(ctx, vertex, fragment, geometry);

    VK_CALL_I(vkDestroyShaderModule, ctx->GetDevice(), vertex, nullptr);
    VK_CALL_I(vkDestroyShaderModule, ctx->GetDevice(), fragment, nullptr);
    VK_CALL_I(vkDestroyShaderModule, ctx->GetDevice(), geometry, nullptr);

    return pipeline;
  }
};

}  // namespace skity

#endif  // SKITY_SRC_RENDER_HW_VK_VK_PIPELINE_WRAPPER_HPP