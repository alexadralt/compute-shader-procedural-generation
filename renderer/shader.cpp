#include "shader.h"

#include <Windows.h>
#include <atlbase.h>
#include <dxc/dxcapi.h>
#include <SDL3/SDL_stdinc.h>

#include <print>
#include <codecvt>
#include <array>
#include <filesystem>
#include <format>

void rdr::Shader::destroy()
{
    if (m_vk_shader_module != VK_NULL_HANDLE) {
#if LOG_RENDERER_OBJECT_NAMES
        std::println("destroying shader module {}...", m_name);
#else
        std::println("destroying shader module...");
#endif
        vkDestroyShaderModule(m_device->vk_device(), m_vk_shader_module, nullptr);
    }
}

std::optional<rdr::Shader> rdr::Shader::create_from_source_file(const Device& device, const std::string& source_path, Shader_Type shader_type)
{
    std::println("compiling shader {}...", source_path);

    if (!std::filesystem::exists(source_path)) {
        std::println("Error: path {} does not exist", std::filesystem::absolute(source_path).string());
        return std::nullopt;
    }

    HRESULT result;

    CComPtr<IDxcLibrary> library;
    result = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
    if (FAILED(result)) {
        std::println("Could not create dxc library instance: {}", result);
        return std::nullopt;
    }

    CComPtr<IDxcCompiler3> compiler;
    result = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    if (FAILED(result)) {
        std::println("Could not create dxc compiler instance: {}", result);
        return std::nullopt;
    }

    CComPtr<IDxcUtils> utils;
    result = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
    if (FAILED(result)) {
        std::println("Could not create dxc utils instance: {}", result);
        return std::nullopt;
    }

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring source_path_wide = converter.from_bytes(source_path);
    
    uint32_t code_page = DXC_CP_UTF8;
    CComPtr<IDxcBlobEncoding> source_blob;
    result = utils->LoadFile(source_path_wide.data(), &code_page, &source_blob);
    if (FAILED(result)) {
        std::println("Could not load source file: {}", result);
        return std::nullopt;
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
            return std::nullopt;
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

    DxcBuffer buffer{};
    buffer.Encoding = DXC_CP_UTF8;
    buffer.Ptr = source_blob->GetBufferPointer();
    buffer.Size = source_blob->GetBufferSize();

    CComPtr<IDxcResult> compilation_result;
    result = compiler->Compile(&buffer, arguments.data(), static_cast<Uint32>(arguments.size()), nullptr, IID_PPV_ARGS(&compilation_result));
    if (SUCCEEDED(result)) {
        compilation_result->GetStatus(&result);
        if (FAILED(result)) {
            CComPtr<IDxcBlobEncoding> error_blob;
            result = compilation_result->GetErrorBuffer(&error_blob);
            std::println("Could not compile shader: {}", SUCCEEDED(result)
                                                         ? static_cast<const char*>(error_blob->GetBufferPointer())
                                                         : std::format("error when getting error buffer: {}", result));
            return std::nullopt;
        }
    }
    else {
        std::println("Could not run the compiler: {}", result);
        return std::nullopt;
    }

    Shader shader;
    shader.m_device = &device;
    
#if LOG_RENDERER_OBJECT_NAMES
    shader.m_name = source_path;
#endif

    CComPtr<IDxcBlob> code;
    compilation_result->GetResult(&code);
    
    VkShaderModuleCreateInfo shader_module_CI{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code->GetBufferSize(),
        .pCode = static_cast<Uint32*>(code->GetBufferPointer()),
    };

    VkResult vk_result;
    vk_result = vkCreateShaderModule(device.vk_device(), &shader_module_CI, nullptr, &shader.m_vk_shader_module);
    if (vk_result != VK_SUCCESS) {
        std::println("Could create shader module: {}", static_cast<Sint32>(vk_result));
        return std::nullopt;
    }

    return shader;
}
