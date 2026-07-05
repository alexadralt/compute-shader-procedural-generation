#include "shader_compiler.h"

#include <Windows.h>
#include <atlbase.h>
#include <dxc/dxcapi.h>
#include <SDL3/SDL_stdinc.h>
#include <spirv_reflect.h>

#include <print>
#include <array>
#include <filesystem>
#include <format>
#include <cassert>

std::wstring rdr::Shader_Compiler::convert_to_utf_16(std::string_view input)
{
    Sint32 wide_size = MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<Sint32>(input.size()), NULL, 0);
    
    std::wstring output;
    output.resize(wide_size);
    
    MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<Sint32>(input.size()), output.data(), wide_size);
    return output;
}

rdr::Shader_Compiler::~Shader_Compiler()
{
    if (m_dxc_utils || m_dxc_compiler || m_dxc_library) {
        std::println("destroying shader compiler...");

        if (m_dxc_utils) {
            m_dxc_utils->Release();
        }

        if (m_dxc_compiler) {
            m_dxc_compiler->Release();
        }

        if (m_dxc_library) {
            m_dxc_library->Release();
        }
    }
}

bool rdr::Shader_Compiler::create(Shader_Compiler& out_shader_compiler)
{
    std::println("creating shader compiler...");
    
    Shader_Compiler compiler;
    HRESULT result;

    result = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&compiler.m_dxc_library));
    if (FAILED(result)) {
        std::println("Could not create dxc library instance: {}", result);
        return false;
    }

    result = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler.m_dxc_compiler));
    if (FAILED(result)) {
        std::println("Could not create dxc compiler instance: {}", result);
        return false;
    }

    result = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&compiler.m_dxc_utils));
    if (FAILED(result)) {
        std::println("Could not create dxc utils instance: {}", result);
        return false;
    }

    out_shader_compiler = std::move(compiler);
    return true;
}

bool rdr::Shader_Compiler::compile_from_source_file(const Device& device, std::string_view source_path, Shader_Type shader_type, Shader& out_shader)
{
    std::println("compiling shader {}...", source_path);

    if (!std::filesystem::exists(source_path)) {
        std::println("Error: path {} does not exist", std::filesystem::absolute(source_path).string());
        return false;
    }

    std::wstring source_path_wide = convert_to_utf_16(source_path);

    HRESULT result;

    uint32_t code_page = DXC_CP_UTF8;
    CComPtr<IDxcBlobEncoding> source_blob;
    result = m_dxc_utils->LoadFile(source_path_wide.data(), &code_page, &source_blob);
    if (FAILED(result)) {
        std::println("Could not load source file: {}", result);
        return false;
    }

    const wchar_t* shader_profile;
    switch (shader_type) {
        case Shader_Type::Vertex:
        {
            shader_profile = L"vs_6_1";
        } break;
        case Shader_Type::Fragment:
        {
            shader_profile = L"ps_6_1";
        } break;
        case Shader_Type::Compute:
        {
            shader_profile = L"cs_6_1";
        } break;
        default:
        {
            assert(false);
            return false;
        }
    }

    std::array<const wchar_t*, 5> arguments = {
        // Shader main entry point
        L"-E", L"main",
        // Shader target profile
        L"-T", shader_profile,
        // Compile to SPIRV
        L"-spirv"
    };

    DxcBuffer buffer{
        .Ptr = source_blob->GetBufferPointer(),
        .Size = source_blob->GetBufferSize(),
        .Encoding = DXC_CP_UTF8,
    };

    CComPtr<IDxcResult> compilation_result;
    result = m_dxc_compiler->Compile(&buffer, arguments.data(), static_cast<Uint32>(arguments.size()), nullptr, IID_PPV_ARGS(&compilation_result));
    if (SUCCEEDED(result)) {
        HRESULT compilation_get_status_result = compilation_result->GetStatus(&result);
        if (FAILED(compilation_get_status_result)) {
            std::println("Could not get compilation status: {}", compilation_get_status_result);
            return false;
        }

        if (FAILED(result)) {
            CComPtr<IDxcBlobEncoding> error_blob;
            result = compilation_result->GetErrorBuffer(&error_blob);
            std::println("Could not compile shader: {}",
                SUCCEEDED(result)
                ? static_cast<const char*>(error_blob->GetBufferPointer())
                : std::format("error when getting error buffer: {}", result));
            return false;
        }
    }
    else {
        std::println("Could not run the compiler: {}", result);
        return false;
    }

    CComPtr<IDxcBlob> code;
    compilation_result->GetResult(&code);

    VkShaderModuleCreateInfo shader_module_CI{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code->GetBufferSize(),
        .pCode = static_cast<Uint32*>(code->GetBufferPointer()),
    };

    VkShaderModule shader_module;
    VkResult vk_result = vkCreateShaderModule(device.vk_device(), &shader_module_CI, nullptr, &shader_module);
    if (vk_result != VK_SUCCESS) {
        std::println("Could not create shader module: {}", static_cast<Sint32>(vk_result));
        return false;
    }

    std::unique_ptr<SpvReflectShaderModule> spv_shader_module = std::make_unique<SpvReflectShaderModule>();
    SpvReflectResult spv_result = spvReflectCreateShaderModule(code->GetBufferSize(), code->GetBufferPointer(), spv_shader_module.get());
    if (spv_result != SPV_REFLECT_RESULT_SUCCESS) {
        std::println("Could not create spriv_reflect shader module: {}", static_cast<Sint32>(spv_result));
        return false;
    }

#if LOG_RENDERER_OBJECT_NAMES
    Shader shader(device, shader_module, std::move(spv_shader_module), std::string(source_path));
#else
    Shader shader(device, shader_module, std::move(spv_shader_module));
#endif
    out_shader = std::move(shader);
    return true;
}
