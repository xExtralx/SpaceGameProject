#pragma once
#include <glad/glad.h>
#include <string>
#include <iostream>

class Shader
{
public:
    unsigned int ID = 0;

    // ------------------------------------------------------------------------
    // Constructeur : prend directement le code source des shaders
    Shader(const std::string& vertexSource, const std::string& fragmentSource)
    {
        unsigned int vertex = compile(GL_VERTEX_SHADER, vertexSource, "VERTEX");
        unsigned int fragment = compile(GL_FRAGMENT_SHADER, fragmentSource, "FRAGMENT");

        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkErrors(ID, "PROGRAM");

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    // ------------------------------------------------------------------------
    void use() const { glUseProgram(ID); }

    // ------------------------------------------------------------------------
    void setBool(const std::string& name, bool value) const {
        glUniform1i(getLoc(name), (int)value);
    }

    void setInt(const std::string& name, int value) const {
        glUniform1i(getLoc(name), value);
    }

    void setFloat(const std::string& name, float value) const {
        glUniform1f(getLoc(name), value);
    }

    void setVec2(const std::string& name, float x, float y) const {
        glUniform2f(getLoc(name), x, y);
    }

    void setVec3(const std::string& name, float x, float y, float z) const {
        glUniform3f(getLoc(name), x, y, z);
    }

    void setVec4(const std::string& name, float x, float y, float z, float w) const {
        glUniform4f(getLoc(name), x, y, z, w);
    }

    // ------------------------------------------------------------------------
    ~Shader() {
        if (ID != 0) glDeleteProgram(ID);
    }

private:
    // Compile un shader OpenGL à partir d'une source
    static unsigned int compile(GLenum type, const std::string& source, const std::string& name)
    {
        unsigned int shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        checkErrors(shader, name);
        return shader;
    }

    // Vérifie les erreurs de compilation / linkage
    static void checkErrors(unsigned int object, const std::string& type)
    {
        int success = 0;
        char log[1024];

        if (type != "PROGRAM") {
            glGetShaderiv(object, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(object, 1024, nullptr, log);
                std::cerr << "❌ Shader compilation failed (" << type << "):\n" << log << std::endl;
            }
        } else {
            glGetProgramiv(object, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(object, 1024, nullptr, log);
                std::cerr << "❌ Program linking failed:\n" << log << std::endl;
            }
        }
    }

    int getLoc(const std::string& name) const {
        return glGetUniformLocation(ID, name.c_str());
    }
};
