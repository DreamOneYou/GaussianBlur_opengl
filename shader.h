#ifndef __SHADER_H__
#define __SHADER_H__

#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Shader
{
    public:
        // constructor
        Shader(const char* vertex_file_path, const char* frag_file_path);

        GLuint getProgramID();

        void use();
        // sets a boolean value to a bool uniform variable
        void setBool(const std::string &name, bool value);
        // sets a int value to a int uniform variable
        void setInt(const std::string &name, int value);
        // sets a float value to a float uniform variable
        void setFloat(const std::string &name, float value);

    private:
        GLuint ID;
        void checkCompileErrors(unsigned int shader, std::string type);
};

#endif