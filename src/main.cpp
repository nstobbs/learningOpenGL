#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

//Adding some macros for ease of life and for error handing within OpenGL
#define PRINT(x) std::cout << x << std::endl;
#define ASSERT(x) if (!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))

//Defining a function for clearing OpenGL errors before checking the error state
static void GLClearError()
{
    while (glGetError() != GL_NO_ERROR);
}

//Defining a function that prints out OpenGL errors before its hits the breaking point
static bool GLLogCall(const char* function, const char* file, int line)
{
    while (GLenum error = glGetError())
    {
        PRINT("[OpenGL ERROR] (" << error << "): " << function << " " << file << ":" << " on line " << line );
        return false;
    }
    return true;
}

//Defining a class to source our vertex and fragment shaders source code data
struct ShaderProgramSource
{
    std::string VertexSource;
    std::string FragmentSource;
};

//Defining a function that can read our shader file for both vertex and fragment shaders
static ShaderProgramSource ParseShader(const std::string& filepath)
{
    std::ifstream stream(filepath);

    enum class ShaderType
    {
        NONE = -1, VERTEX = 0, FRAGMENT = 1
    };

    std::string line;
    std::stringstream ss[2];
    ShaderType type = ShaderType::NONE;

    while (getline(stream, line))
    {
        if (line.find("#shader") != std::string::npos)
        {
            if (line.find("vertex") != std::string::npos)
            {
                type = ShaderType::VERTEX;
            }
            else if (line.find("fragment") != std::string::npos)
            {
                type = ShaderType::FRAGMENT;
            }
        }
        else
        {
            ss[(int)type] << line << "\n";
        }
    }

    return { ss[0].str(), ss[1].str() };
}

// Function for compiling the shader soruce code 
static unsigned int CompileShader(unsigned int type, const std::string& source)
{
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1 , &src, nullptr);
    glCompileShader(id);

    // Shader Compile Error Handling
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        int lenght;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &lenght);
        char* message = (char*)alloca(lenght * sizeof(char));
        glGetShaderInfoLog(id, lenght, &lenght, message);
        std::cout << "Faild to compile" << (type == GL_VERTEX_SHADER ? " vectex" : " fragment") << " shader!" << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(id);

        return 0;
    }

    return id;
};

// Function for creating a new program and adding the compiled shader code to the program 
static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader) 
{
    unsigned int program =  glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
};

//Function that doesnt work for changing the colour 
float animatedFloat(float color_channel, float increment)
{

    if (color_channel > 1.0f)
    {
        increment = -0.05f;
    }
    else if (color_channel < 0.0f)
    {
        increment = 0.05f;
    }

    return color_channel += increment;
};

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;


    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Learning OpenGL!", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);

    // Checking GLEW is not erroring 
    if (glewInit() != GLEW_OK) {

        std::cout << "Error" << std::endl;
    };

    // Print GPU version and GL verison
    PRINT(glGetString(GL_VERSION));

    // Defining an array to store the triangle vertexs positions, formated by 2 bytes.
    float positions[] = {
       -0.5f, -0.5,     //0
        0.5f, -0.5f,    //1
        0.5f,  0.5f,    //2
       -0.5f,  0.5f     //3
    };

    //Difining an array to store data on how to create a triangle
    unsigned int indices[] = {
        0,1,2,
        2,3,0
    };

    // Defining a array buffer and storing the triangle positions data into the buffer 
    unsigned int buffer;
    GLCall(glGenBuffers(1, &buffer));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, buffer));
    GLCall(glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), positions, GL_STATIC_DRAW)); // storing the trinagle data to the buffer

    // Defining the layout of the buffer, e.g. every two bytes within positions is a 2d vertex position and enabling the vertex attrib
    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0));

    //Defining a array buffer and using to for index data about how to draw the triangles
    unsigned int indexBufferObject;
    GLCall(glGenBuffers(1, &indexBufferObject));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferObject));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW));

    ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");

    PRINT("Creating Shaders From File res/shaders/Basic...");
    PRINT("VertexSource");
    PRINT(source.VertexSource);
    PRINT("Fragment Source");
    PRINT(source.FragmentSource);

    // Making and using the newly created shader
    unsigned int shader = CreateShader(source.VertexSource, source.FragmentSource);
    GLCall(glUseProgram(shader));

    //Defining the u_Color as a uniform to use inside of c++ for the shader
    GLCall(int location = glGetUniformLocation(shader, "u_Color"));
    ASSERT(location != -1);
    GLCall(glUniform4f(location, 0.7f, 0.0f, 0.3, 1.0f));

    float r = 0.5f;
    float b = 0.3f;
    float g = 0.9f;

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        GLCall(glClear(GL_COLOR_BUFFER_BIT));
        
        r = animatedFloat(r, 0.05f);
        g = animatedFloat(g, 0.15f);
        b = animatedFloat(b, 0.2f);

        PRINT("Red = " << r);
        PRINT("Green = " << g);
        PRINT("Blue = " << b);

        GLCall(glUniform4f(location, r, g, b, 1.0f));
        GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr)); // Drawing the triangles to the screen

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    GLCall(glDeleteProgram(shader));

    glfwTerminate();
    return 0;
}