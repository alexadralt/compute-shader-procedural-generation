#pragma once

#include "shader.h"

#include <utility>
#include <optional>
#include <string>
#include <string_view>

class IDxcLibrary;
class IDxcCompiler3;
class IDxcUtils;

namespace rdr {
    class Shader_Compiler {
        IDxcLibrary* m_dxc_library;
        IDxcCompiler3* m_dxc_compiler;
        IDxcUtils* m_dxc_utils;

        Shader_Compiler(const Shader_Compiler& other) = delete;
        Shader_Compiler& operator=(const Shader_Compiler& other) = delete;

        static std::wstring convert_to_utf_16(std::string_view input);
    public:
        Shader_Compiler() : m_dxc_library(nullptr),
                            m_dxc_compiler(nullptr),
                            m_dxc_utils(nullptr) {}
        ~Shader_Compiler();

        static std::optional<Shader_Compiler> create();

        Shader_Compiler(Shader_Compiler&& other) noexcept : m_dxc_library(other.m_dxc_library),
                                                            m_dxc_compiler(other.m_dxc_compiler),
                                                            m_dxc_utils(other.m_dxc_utils)
        {
            new (&other) Shader_Compiler();
        }

        Shader_Compiler& operator=(Shader_Compiler&& other) noexcept {
            this->~Shader_Compiler();
            new (this) Shader_Compiler(std::move(other));
            return *this;
        }

        std::optional<Shader> compile_from_source_file(const Device& device, std::string_view source_path, Shader_Type shader_type);
    };
}
