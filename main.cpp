// Incluir cabeçalhos padrão
#include <stdio.h>
#include <stdlib.h>

// Incluir GLEW
#include <GL/glew.h>

// Incluir GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Incluir GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;

// Incluir o carregador de OBJ
#include "common/tiny_obj_loader.h"

// Inclua os shader
#include <common/shader.hpp>

GLuint VertexArrayID;
GLuint programID;
GLuint MatrixID;
glm::mat4 Projection;
glm::mat4 View;
GLuint hangarColorBuffer;

GLuint colorbuffer;
std::vector<float> colors;

// Buffers 
GLuint falconVAO, hangarVAO;
GLuint vertexbuffer, normalbuffer;
GLuint hangarVertexBuffer, hangarNormalBuffer;

glm::mat4 MVP;

// Dimensões da janela
GLint WindowWidth = 1024;
GLint WindowHeight = 768;

//Para movimento da nave
float modelX = 0.0f;
float modelZ = 0.0f;
float modelRotationY = 0.0f;
float modelSpeed = 0.1f;

// Dados do modelo
std::vector<float> vertices;
std::vector<float> normals;
std::vector<float> hangarVertices;
std::vector<float> hangarNormals;

//Geração das cores
void gerarCinzentos(std::vector<float>& out_colors, size_t vertex_count, float baseGrey) {
    for (size_t i = 0; i < vertex_count; ++i) {
        float grey = baseGrey; 
        out_colors.push_back(grey); 
        out_colors.push_back(grey); 
        out_colors.push_back(grey); 
    }
}


// Função para carregar o modelo OBJ
bool loadOBJ(const char* path, std::vector<float>& out_vertices, std::vector<float>& out_normals)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path);
    if (!ret) {
        fprintf(stderr, "O Obj não carregou bem: %s\n", path);
        return false;
    }

    // Extrair os vértices e normais
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            out_vertices.push_back(attrib.vertices[3 * index.vertex_index + 0]);
            out_vertices.push_back(attrib.vertices[3 * index.vertex_index + 1]);
            out_vertices.push_back(attrib.vertices[3 * index.vertex_index + 2]);
            out_normals.push_back(attrib.normals[3 * index.normal_index + 0]);
            out_normals.push_back(attrib.normals[3 * index.normal_index + 1]);
            out_normals.push_back(attrib.normals[3 * index.normal_index + 2]);
        }
    }
    return true;
}


void transferDataToGPUMemory(void)
{
    // VAO
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);
    
    //Carregar shaders program
    programID = LoadShaders("TransformVertexShader.vertexshader", "ColorFragmentShader.fragmentshader");
	
    //Carregar a nave
    if (!loadOBJ("falcon.obj", vertices, normals)) {
        fprintf(stderr, "Erro ao carreagar a nave\n");
        exit(-1);
    }
    
    // Cores para a nave
    gerarCinzentos(colors, vertices.size() / 3, 0.7f);
	
    // Vertex buffer da nave
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    // Normal buffer da nave
    glGenBuffers(1, &normalbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), &normals[0], GL_STATIC_DRAW);

    // Color buffer da nave
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), &colors[0], GL_STATIC_DRAW);

    // Carregar o hangar
    if (!loadOBJ("hangar.obj", hangarVertices, hangarNormals)) {
        fprintf(stderr, "Erro ao carregar o hangar\n");
        exit(-1);
    }

    // Cores para o hangar
    std::vector<float> hangarColors;
    gerarCinzentos(hangarColors, hangarVertices.size() / 3, 0.5f);

    // Vertex buffer do hangar
    glGenBuffers(1, &hangarVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, hangarVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, hangarVertices.size() * sizeof(float), &hangarVertices[0], GL_STATIC_DRAW);

    // Normal buffer do hangar
    glGenBuffers(1, &hangarNormalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, hangarNormalBuffer);
    glBufferData(GL_ARRAY_BUFFER, hangarNormals.size() * sizeof(float), &hangarNormals[0], GL_STATIC_DRAW);

    // Color buffer do hangar
    glGenBuffers(1, &hangarColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, hangarColorBuffer);
    glBufferData(GL_ARRAY_BUFFER, hangarColors.size() * sizeof(float), &hangarColors[0], GL_STATIC_DRAW);
    
    //Carregar os dados
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glBindVertexArray(0);
}


//Funcao para o movimento da nave
void controloNave(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_D) {
            modelX += modelSpeed;
        }
        if (key == GLFW_KEY_A) {
            modelX -= modelSpeed;
        }
        if (key == GLFW_KEY_W) {
            modelZ += modelSpeed;
        }
        if (key == GLFW_KEY_S) {
            modelZ -= modelSpeed;
        }
        if (key == GLFW_KEY_E) {
            modelRotationY += 1.0f;
        }
        if (key == GLFW_KEY_Q) {
            modelRotationY -= 1.0f;
        }
    }
}

//Funcao para desenhar o modelo recebido
void drawModel(GLuint vertexbuffer, GLuint normalbuffer, GLuint colorbuffer, size_t vertex_count, glm::mat4 modelMatrix)
{
    glm::mat4 ModelViewMatrix = View * modelMatrix;
    glm::mat3 NormalMatrix = glm::mat3(glm::transpose(glm::inverse(ModelViewMatrix)));

    MVP = Projection * ModelViewMatrix;

    //Envia matriz aos shaders
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, value_ptr(MVP));
    GLuint ModelViewMatrixID = glGetUniformLocation(programID, "ModelViewMatrix");
    GLuint NormalMatrixID = glGetUniformLocation(programID, "NormalMatrix");
    glUniformMatrix4fv(ModelViewMatrixID, 1, GL_FALSE, value_ptr(ModelViewMatrix));
    glUniformMatrix3fv(NormalMatrixID, 1, GL_FALSE, value_ptr(NormalMatrix));


    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);


    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Desenhar o modelo
    glDrawArrays(GL_TRIANGLES, 0, vertex_count);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

int main(void)
{
    // Initialize GLFW and create window
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(WindowWidth, WindowHeight, "3D Models", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    
    glfwMakeContextCurrent(window);
    glewInit();
    glfwSetKeyCallback(window, controloNave);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Transfer data to GPU
    transferDataToGPUMemory();
    
    // Obtain the MVP uniform ID
    MatrixID = glGetUniformLocation(programID, "MVP");

    //Escala dos hangar 
    float hangarScale = 0.045f;
    float hangarRotationAngle = 180.0f;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(programID);
        
        //Parametros da iluminação
		glm::vec3 lightPosition(4.0f, 4.0f, 4.0f);
		glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
		glm::vec3 ambientColor(0.2f, 0.2f, 0.2f);
		float shininess = 32.0f;
		float specularStrength = 0.5f;

		GLuint lightPosID = glGetUniformLocation(programID, "lightPosition_cameraSpace");
		GLuint lightColorID = glGetUniformLocation(programID, "lightColor");
		GLuint ambientColorID = glGetUniformLocation(programID, "ambientColor");
		GLuint shininessID = glGetUniformLocation(programID, "shininess");
		GLuint strengthID = glGetUniformLocation(programID, "strength");

		//Passar dados da iluminacao para o shader
		glm::vec3 lightPos_cameraSpace = glm::vec3(View * glm::vec4(lightPosition, 1.0));
		glUniform3fv(lightPosID, 1, glm::value_ptr(lightPos_cameraSpace));
		glUniform3fv(lightColorID, 1, glm::value_ptr(lightColor));
		glUniform3fv(ambientColorID, 1, glm::value_ptr(ambientColor));
		glUniform1f(shininessID, shininess);
		glUniform1f(strengthID, specularStrength);


        Projection = glm::perspective(glm::radians(90.0f), (float)WindowWidth / (float)WindowHeight, 0.1f, 100.0f);
        View = glm::lookAt(glm::vec3(3, 5, 16), glm::vec3(0, 0, 0), glm::vec3(0, 0.5, 0));

        // Desenhar 1 hangar 
        drawModel(hangarVertexBuffer, hangarNormalBuffer, hangarColorBuffer, hangarVertices.size() / 3,
                  glm::translate(glm::mat4(1.0f), glm::vec3(-20.0f, 0.0f, -5.0f)) *
                  glm::scale(glm::mat4(1.0f), glm::vec3(hangarScale, hangarScale, hangarScale)));

        // Desenhar segundo hangar
        drawModel(hangarVertexBuffer, hangarNormalBuffer, hangarColorBuffer, hangarVertices.size() / 3,
          glm::translate(glm::mat4(1.0f), glm::vec3(20.0f, 0.0f, -5.0f)) *
          glm::rotate(glm::mat4(1.0f), glm::radians(hangarRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)) *
          glm::scale(glm::mat4(1.0f), glm::vec3(hangarScale, hangarScale, hangarScale)));

        // Desenhar nave
        glm::mat4 falconModel = glm::translate(glm::mat4(1.0f), glm::vec3(modelX, 0.0f, modelZ)); 
		falconModel = glm::rotate(falconModel, glm::radians(modelRotationY), glm::vec3(0.0f, 1.0f, 0.0f));
		drawModel(vertexbuffer, normalbuffer, colorbuffer, vertices.size() / 3, falconModel);
		
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &normalbuffer);
    glDeleteBuffers(1, &hangarVertexBuffer);
    glDeleteBuffers(1, &hangarNormalBuffer);
    glDeleteBuffers(1, &colorbuffer);
    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteProgram(programID);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}


