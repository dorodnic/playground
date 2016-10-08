#include "shader.h"

#include "types.h"

#ifdef WIN32
    #define USEGLEW
    #include <GL/glew.h>
#endif

#define GLFW_INCLUDE_GLU
#define GLFW_INCLUDE_GLEXT
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#include "../easyloggingpp/easylogging++.h"

Shader::Shader(const std::string& filename, ShaderType type)
{
    auto lambda = [&](){
        switch(type)
        {
            case ShaderType::Vertex: return GL_VERTEX_SHADER;
            case ShaderType::Fragment: return GL_FRAGMENT_SHADER;
            default:
                throw std::runtime_error("Unknown shader type!");
        }
    };
    const auto gl_type = lambda();
    
    GLuint shader_id = glCreateShader(gl_type);

    auto shader_code = read_all_text(filename);
    LOG(INFO) << "Compiling shader " << filename << "...";
    
    char const * source_ptr = shader_code.c_str();
    int length = shader_code.size();
    glShaderSource(shader_id, 1, &source_ptr, &length);
    
    glCompileShader(shader_id);

    GLint result;
    int log_length;
    
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
    if ((result == GL_FALSE) && (log_length > 0)){
        std::vector<char> error_message(log_length+1);
        glGetShaderInfoLog(shader_id, log_length, NULL, &error_message[0]);
        std::string error(&error_message[0]);
        LOG(ERROR) << error;
        glDeleteShader(shader_id);
        throw std::runtime_error(error);
    }
    
    LOG(INFO) << "Shader " << filename << " [OK]";
    
    _id = shader_id;
}

Shader::~Shader()
{
    glDeleteShader(_id);
}

ShaderProgram::ShaderProgram()
{
    GLuint program_id = glCreateProgram();
    _id = program_id;
}
ShaderProgram::~ShaderProgram()
{
    glUseProgram(0);
	glDeleteProgram(_id);
}

void ShaderProgram::attach(const Shader& shader)
{
    _shaders.push_back(&shader);
}
void ShaderProgram::link()
{
    for(auto ps : _shaders)
    {
        glAttachShader(_id, ps->get_id());
    }
    
    LOG(INFO) << "Linking the shader program...";
    glLinkProgram(_id);
    
    GLint result;
    int log_length;

    glGetProgramiv(_id, GL_LINK_STATUS, &result);
    glGetProgramiv(_id, GL_INFO_LOG_LENGTH, &log_length);
    if ((result == GL_FALSE) && (log_length > 0)){
        std::vector<char> error_message(log_length+1);
        glGetProgramInfoLog(_id, log_length, NULL, &error_message[0]);
        std::string error(&error_message[0]);
        LOG(ERROR) << error;
        for(auto ps : _shaders)
        {
            glDetachShader(_id, ps->get_id());
        }
        throw std::runtime_error(error);
    }
    
    LOG(INFO) << "Validating the shader program...";
    glValidateProgram(_id);

    glGetProgramiv(_id, GL_VALIDATE_STATUS, &result);
    glGetProgramiv(_id, GL_INFO_LOG_LENGTH, &log_length);
    if ((result == GL_FALSE) && (log_length > 0)){
        std::vector<char> error_message(log_length+1);
        glGetProgramInfoLog(_id, log_length, NULL, &error_message[0]);
        std::string error(&error_message[0]);
        LOG(ERROR) << error;
        for(auto ps : _shaders)
        {
            glDetachShader(_id, ps->get_id());
        }
        throw std::runtime_error(error);
    }
    
    LOG(INFO) << "Shader Program ready";
    
    for(auto ps : _shaders)
    {
        glDetachShader(_id, ps->get_id());
    }
    _shaders.clear();
}

void ShaderProgram::begin() const
{
    glUseProgram(_id);
}
void ShaderProgram::end() const
{
    glUseProgram(0);
}

std::unique_ptr<ShaderProgram> ShaderProgram::load(
                                const std::string& vertex_shader,
                                const std::string& fragment_shader)
{
    std::unique_ptr<ShaderProgram> res(new ShaderProgram());
    Shader vertex(vertex_shader, ShaderType::Vertex);
    Shader fragment(fragment_shader, ShaderType::Fragment);
    res->attach(vertex);
    res->attach(fragment);
    res->link();
    return std::move(res);
}
