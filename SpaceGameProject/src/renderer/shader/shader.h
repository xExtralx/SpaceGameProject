#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <string>
#include <iostream>
#include "../../utils.h" // pour la fonction LoadTextFile

class Shader {
public:
    unsigned int ID = 0;

    Shader() = default;

    // --- Constructeur depuis code source (déjà chargé) ---
    Shader(const std::string& vertexCode, const std::string& fragmentCode) {
        compile(vertexCode, fragmentCode);
    }

    // --- Chargement direct depuis fichiers ---
    static Shader FromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
        std::string vertexCode   = FileManager::LoadTextFile(vertexPath);
        std::string fragmentCode = FileManager::LoadTextFile(fragmentPath);
        return Shader(vertexCode, fragmentCode);
    }

    // --- Utilisation du shader ---
    void use() const {
        glUseProgram(ID);
    }

    // --- Nettoyage ---
    void destroy() {
        if (ID != 0) {
            glDeleteProgram(ID);
            ID = 0;
        }
    }

    // --- Uniform setters ---
    void setBool(const std::string& name, bool value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    void setInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setVec2(const std::string& name, float x, float y) const {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }

    void setVec3(const std::string& name, float x, float y, float z) const {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }

    void setVec4(const std::string& name, float x, float y, float z, float w) const {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }

private:
    // --- Compilation du shader ---
    void compile(const std::string& vertexCode, const std::string& fragmentCode) {
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // 1. Vertex shader
        unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, nullptr);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // 2. Fragment shader
        unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, nullptr);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // 3. Shader program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // 4. Supprimer les shaders
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    // --- Vérification des erreurs de compilation ---
    void checkCompileErrors(unsigned int shader, const std::string& type) {
        int success;
        char infoLog[1024];

        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                std::cerr << "❌ Shader compilation error (" << type << "):\n"
                          << infoLog << std::endl;
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
                std::cerr << "❌ Program linking error:\n"
                          << infoLog << std::endl;
            }
        }
    }
};

#endif
