#include <iostream>
#include <string>
#include <stb_image.h>
#include <shader.h>

#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
using namespace glm;
#define GLFW_TRUE 1

static GLFWwindow* win = NULL;
int window_width = 1024;
int window_height = 768;
int texture_width, texture_height;

void errorCallback(int error, const char* description)
{
    fprintf(stderr, "Error(%d): %s\n", error, description);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
    glfwSetWindowSize(window, width, height);
}

// configuration about window settings
void configure(int major, int minor, GLboolean forward_compat, GLenum profile)
{
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, forward_compat);
    glfwWindowHint(GLFW_OPENGL_PROFILE, profile);
}

// does all the initializations necessary
void initialize(int width, int height, const char *windowName)
{
    if (!glfwInit())
    {
        std::cout<<"Failed to initialize GLFW\n"<<std::endl;
        exit(-1);
    }

    configure(3, 3, GL_TRUE, GLFW_OPENGL_CORE_PROFILE);
    glfwSetErrorCallback(errorCallback);

    win = glfwCreateWindow(width, height, windowName, NULL, NULL);
    if (!win)
    {
        std::cerr << "Failed to open GLFW window.\n" << std::endl;
        glfwTerminate();
        exit(-1);
    }

    glfwSetWindowSizeCallback(win, window_size_callback);
    glfwMakeContextCurrent(win);
    glewExperimental= true;

    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cerr << "GLEW initialization failed.\n" << std::endl;
        exit(-1);
    }

    glfwSetInputMode(win, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetKeyCallback(win, keyCallback);
}

Shader loadShaders(const char *vertexShaderName, const char *fragmentShaderName)
{
    Shader ourShader = Shader(vertexShaderName, fragmentShaderName);
    return ourShader;
}

// reads and loads texture
void loadTexture(const char *fileName, GLuint& texture, int& width, int& height, const int type )
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    stbi_set_flip_vertically_on_load(true);  

    // Loading Texture
    int nrChannels;
    unsigned char *data = stbi_load(fileName, &width, &height, &nrChannels, 0);
    
    // generating textures
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cerr << "Failed to load texture image: " << stbi_failure_reason() << std::endl;
        exit(-1);
    }
    stbi_image_free(data);
}

// naive implementation O(n^2)
// uses naive shader 
void naive(Shader &shader, GLuint &texture, GLuint &VAO)
{
    glViewport( 0, 0, texture_width, texture_height);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, texture);
    shader.use();
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glViewport( 0, 0, window_width, window_height);
}

// two pass gaussian filter - O(2n)
// uses two-pass shader
void separated(Shader &shader1, Shader &shader2, GLuint& FBO1, GLuint& FBO2, GLuint& intermediate_texture, GLuint& filtered_texture, GLuint& texture, GLuint& VAO, GLuint& dirLoc)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glViewport( 0, 0, texture_width, texture_height);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO1); // bind Framebuffer1
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader1.use(); // simple texture mapping shader
    glBindTexture(GL_TEXTURE_2D, texture); // color attachment texture
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, FBO2); // bind Framebuffer2
    glBindTexture(GL_TEXTURE_2D, intermediate_texture); // use the texture of the second one
    shader2.use(); // two-pass gauss blur shader
    glUniform2f(dirLoc, 0.0f, 1.0f/float(texture_height)); // vertical
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, filtered_texture); // use the texture of the second one (vertically blurred)
    shader2.use(); // two-pass gauss blur shader
    glUniform2f(dirLoc, 1.0f/float(texture_width), 0.0f); // horizontal
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glViewport( 0, 0, window_width, window_height);
}

// uses two-pass gaussian with bilinear filtering
void separated_bilinear(Shader &shader1, Shader &shader2, GLuint& FBO1, GLuint& FBO2, GLuint& intermediate_texture, GLuint& filtered_texture, GLuint& texture, GLuint& VAO, GLuint& dirLoc)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glViewport( 0, 0, texture_width, texture_height);

    glBindFramebuffer(GL_FRAMEBUFFER, FBO1); // bind Framebuffer0 
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader1.use(); // simple shader for texture mapping
    glBindTexture(GL_TEXTURE_2D, texture); // color attachment texture
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, FBO2); // bind second Framebuffer
    glBindTexture(GL_TEXTURE_2D, intermediate_texture); // use the texture of the first one
    shader2.use(); // two-pass gauss shader with linear filtering
    glUniform2f(dirLoc, 0.0f, 1.0f/float(texture_height)); // vertical
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, filtered_texture); // use the texture of the second one (vertically blurred image)
    shader2.use();  // two-pass gauss shader with linear filtering
    glUniform2f(dirLoc, 1.0f/float(texture_width), 0.0f); // horizontal
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glViewport( 0, 0, window_width, window_height);
}

int main(int argc, char* argv[])
{
    if (argc <= 2)
    {
        std::cerr << "Wrong usage. Correct usage as follows: ./blur <image_to_be_blurred> <implementation_type>." << std::endl;
        std::cerr << "For <implementation_type>, type 1 for naive implementation. 2 or 3 for faster result." << std::endl;
        exit(-1);
    }
    else if (argc > 3)
    {
        std::cerr << "Wrong usage. You have provided extra input." << std::endl;
        std::cerr << "Correct usage as follows: ./blur <image_to_be_blurred> <type_of_implementation>." << std::endl;
        exit(-1);
    }

    int type = atoi(argv[2]);
    if (type < 1 || type > 3)
    {
        std::cerr << "Invalid implementation type. Please choose between 1-3." << std::endl;
        exit(-1);
    }

    initialize(window_width, window_height, "Gaussian Blur");

    // colored and texture vertices
    static const GLfloat vertices[] = {
        // positions            // colors         // texture coordinates
        -1.0f,  1.0f, 1.0f,   0.1f, 0.0f, 0.0f,     0.0f, 1.0f,             // top left
         1.0f,  1.0f, 1.0f,   0.0f, 1.0f, 0.0f,     1.0f, 1.0f,             // top right
         1.0f, -1.0f, 1.0f,   0.0f, 0.0f, 0.1f,     1.0f, 0.0f,             // bottom right
        -1.0f, -1.0f, 1.0f,   1.0f, 0.0f, 0.0f,     0.0f, 0.0f              // bottom left
    };

    static const GLuint indices[] = {
        0, 1, 2, // first triangle
        0, 2, 3  // second triangle
    };

    // simple texture shader
    Shader shader1 = loadShaders("SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader");
    // naive implementation of gaussian filter
    Shader shader2 = loadShaders("SimpleVertexShader.vertexshader", "naive.fragmentshader");
    // separated implementation of gaussian filter
    Shader shader3 = loadShaders("SimpleVertexShader.vertexshader", "separated.fragmentshader");
    // separated with bilinear filtering of gaussian filter
    Shader shader4 = loadShaders("SimpleVertexShader.vertexshader", "linear.fragmentshader");

    GLuint texture;
    texture_width = 0;
    texture_height = 0;

    loadTexture(argv[1], texture, texture_width, texture_height, type);

    window_height = texture_height;
    window_width = texture_width;
    glfwSetWindowSize(win, window_width, window_height);

    GLuint VBO, VAO, EBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    // texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);

    // create the first frame buffer object
    unsigned int FBO1;
    glGenFramebuffers(1, &FBO1);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO1);

    // create a color texture for intermediate buffer holding
    unsigned int intermediate_texture;
    glGenTextures(1, &intermediate_texture);
    glBindTexture(GL_TEXTURE_2D, intermediate_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, intermediate_texture, 0);

    unsigned int FBO2;
    glGenFramebuffers(1, &FBO2);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO2);

    unsigned int filtered_texture;
    glGenTextures(1, &filtered_texture);
    glBindTexture(GL_TEXTURE_2D, filtered_texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, filtered_texture, 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Framebuffer is not complete. " << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GLuint dirLoc_sep = glGetUniformLocation(shader3.getProgramID(), "dir"); // two pass
    GLuint dirLoc_sep_lin = glGetUniformLocation(shader4.getProgramID(), "dir"); // two pass with linear filtering

    // double lastTime = glfwGetTime();
    // int nbFrames = 0;
 
    while(!glfwWindowShouldClose(win))
    {
        // measure time
        // double currentTime = glfwGetTime();
        // nbFrames += 1;
        // // 1 sec passed
        // if (currentTime - lastTime >= 1)
        // {
        //     printf("%f ms/frame\n", 1000/(double(nbFrames)));
        //     nbFrames = 0;
        //     lastTime += 1.0;
        // }

        if (type == 1)
        {
            naive(shader2, texture, VAO);
        }
        else if (type == 2)
        {
            separated(shader1, shader3, FBO1, FBO2, intermediate_texture, filtered_texture, texture, VAO, dirLoc_sep);
        }
        else if (type == 3)
        {
            separated_bilinear(shader1, shader4, FBO1, FBO2, intermediate_texture, filtered_texture, texture, VAO, dirLoc_sep_lin);
        }
        
        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    // Cleanup VBO
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    // Cleanup FBOs
    glDeleteBuffers(1, &FBO1);
    glDeleteBuffers(1, &FBO2);
    // Cleanup textures
    glDeleteTextures(1, &texture);
    glDeleteTextures(1, &intermediate_texture);
    glDeleteTextures(1, &filtered_texture);

    glfwTerminate();
    return 0;
}
