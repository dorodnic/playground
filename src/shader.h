#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

enum class ShaderType
{
    Vertex,
    Fragment    
};

class Shader
{
public:
    Shader(const std::string& filename, ShaderType type);
    ~Shader();
    
    unsigned int get_id() const { return _id; }
    
private:
    unsigned int _id;
};

class ShaderProgram
{
public:
    ShaderProgram();
    ~ShaderProgram();
    
    void attach(const Shader& shader);
    void link();
    
    void begin() const;
    void end() const;
    
    static std::unique_ptr<ShaderProgram> load(
                            const std::string& vertex_shader,
                            const std::string& fragment_shader);
                              
    unsigned int get_id() const { return _id; }

private:
    std::vector<const Shader*> _shaders;
    unsigned int _id;
};