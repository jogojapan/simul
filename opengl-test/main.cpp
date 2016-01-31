/*
  Build as:

  g++ -O0 -W -Wall -Wno-parentheses -std=c++1y -o opengl-test main.cpp -lGL -lGLEW -lSDL2 -lstbi
*/

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cassert>
#include <vector>

#include <SDL2/SDL.h>
#include <GL/glew.h>

#include <glm/glm.hpp>
#define GLM_FORCE_RADIANS
#include <glm/gtx/transform.hpp>

#include <stb_image.h>
#include "./obj_loader.h"

class Vertex
{
public:
  Vertex(const glm::vec3 &pos,
         const glm::vec2 texCoord)
    : pos_(pos),
      texCoord_(texCoord)
  { }

  glm::vec3 pos_;
  glm::vec2 texCoord_;
  glm::vec3 normals_;
};

class Mesh
{
public:
  Mesh(Vertex *vertices,
       unsigned numVertices,
       unsigned int *indices,
       unsigned int numIndices)
    : vertexArrayObject_(),
      vertexArrayBuffers(),
      drawCount_(numIndices)
  {

    IndexedModel model;

    model.positions.reserve(numVertices);
    model.texCoords.reserve(numVertices);

    for (unsigned i = 0 ; i < numVertices ; ++i)
      {
        model.positions.push_back(vertices[i].pos_);
        model.texCoords.push_back(vertices[i].texCoord_);
      }

    model.indices.reserve(numIndices);
    for (unsigned i = 0 ; i < numIndices ; ++i)
      model.indices.push_back(indices[i]);


    init_mesh(model);
  }

  Mesh(const std::string &filename)
  {
    IndexedModel model = OBJModel(filename).ToIndexedModel();
    init_mesh(model);
  }

  virtual ~Mesh()
  {
    glDeleteVertexArrays(1,&vertexArrayObject_);
  }


  void init_mesh(const IndexedModel &model)
  {
    drawCount_ = model.indices.size();

    glGenVertexArrays(1,&vertexArrayObject_);
    glBindVertexArray(vertexArrayObject_);

    // Allocate buffer in GPU memory
    glGenBuffers(NUM_BUFFERS, vertexArrayBuffers);

    glBindBuffer(GL_ARRAY_BUFFER,
                 vertexArrayBuffers[POSITION_VB]);
    // Think of this as moving the data from regular RAM to GPU memory
    glBufferData(GL_ARRAY_BUFFER,
                 model.positions.size() * sizeof(model.positions[0]),
                 model.positions.data(),
                 GL_STATIC_DRAW  // read-only data (may give rise to
                                 // optimizations)
                 );
    // Tell OpenGL how to interpret the data, i.e. how to process it
    // in order to get a sequence of vertexes. In our case, the vertex
    // object only includes the vertex data (i.e. the vec3 data
    // member), so it is very easy.
    glEnableVertexAttribArray(0);
    // How to read the data sequence: Each data point is a sequence of
    // 3 floats, and no superfluous data to skip.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER,
                 vertexArrayBuffers[TEXCOORD_VB]);
    glBufferData(GL_ARRAY_BUFFER,
                 model.positions.size() * sizeof(model.texCoords[0]),
                 model.texCoords.data(),
                 GL_STATIC_DRAW  // read-only data (may give rise to
                                 // optimizations)
                 );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                 vertexArrayBuffers[INDEX_VB]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 model.indices.size() * sizeof(model.indices[0]),
                 model.indices.data(),
                 GL_STATIC_DRAW  // read-only data (may give rise to
                                 // optimizations)
                 );

    glBindVertexArray(0);
  }

  void Draw()
  {
    glBindVertexArray(vertexArrayObject_);
    // glDrawArrays(GL_TRIANGLES, 0, drawCount_);
    glDrawElements(GL_TRIANGLES,
                   drawCount_,
                   GL_UNSIGNED_INT,
                   0);
    glBindVertexArray(0);
  }

private:
  enum
    {
      POSITION_VB,
      TEXCOORD_VB,
      INDEX_VB,

      NUM_BUFFERS // keeping track of the number of enumeration values
    };

  GLuint vertexArrayObject_;
  GLuint vertexArrayBuffers[NUM_BUFFERS];

  // How much of the above data we want to draw
  unsigned int drawCount_;
};

class Transform
{
public:
  Transform(const glm::vec3 &pos = glm::vec3(0,0,0),
            const glm::vec3 &rot = glm::vec3(0,0,0),
            const glm::vec3 &scale = glm::vec3(1.0,1.0,1.0))
    : pos_(pos),
      rot_(rot),
      scale_(scale)
  { };

  glm::mat4 get_model() const
  {
    glm::mat4 pos_matrix   = glm::translate(pos_);
    glm::mat4 scale_matrix = glm::scale(scale_);
    glm::mat4 rotx_matrix  = glm::rotate(rot_.x,glm::vec3(1,0,0));
    glm::mat4 roty_matrix  = glm::rotate(rot_.y,glm::vec3(0,1,0));
    glm::mat4 rotz_matrix  = glm::rotate(rot_.z,glm::vec3(0,0,1));

    glm::mat4 rot_matrix   = rotz_matrix * roty_matrix * rotx_matrix;
    return pos_matrix * rot_matrix * scale_matrix;
  }

public:
  glm::vec3 pos_;
  glm::vec3 rot_;
  glm::vec3 scale_;
};

class Camera
{
public:
  Camera(const glm::vec3 &pos,
         float fov, // field of view
         float aspect,
         float znear, // nearest things we can see
         float zfar)  // farthest things we can see
    : perspective_(glm::perspective(fov,aspect,znear,zfar)),
      pos_(pos),
      forward_(0,0,1),
      up_(0,1,0)
  { }

  glm::mat4 get_view_projection() const
  {
    return perspective_ *
      glm::lookAt(pos_,            // from where I am looking
                  pos_ + forward_, // what I am looking at
                  up_);            // what is upward for me
  }


  glm::mat4 perspective_;
  glm::vec3 pos_;
  glm::vec3 forward_;  // direction the viewer perceives as forward
  glm::vec3 up_;       // direction the viewer perceives as upward
};

class Shader
{
public:
  static std::string LoadShader(const std::string &fileName)
  {
    std::ifstream file;
    std::stringstream buf;
    file.open(fileName.c_str());
    if (!file.is_open())
      {
        std::cerr << "Could not open shader definition '"
                  << fileName
                  << "'"
                  << std::endl;
        return "";
      }
    buf << file.rdbuf();
    file.close();
    return buf.str();
  }

  static void CheckShaderError(GLuint shader, GLuint flag,
                               bool isProgram,
                               const std::string &errorMessage)
  {
    GLint success = 0;
    GLchar error[1024] = { 0 };

    if (isProgram)
      glGetProgramiv(shader,flag,&success);
    else
      glGetShaderiv(shader,flag,&success);

    if (success == GL_FALSE)
      {
        if (isProgram)
          glGetProgramInfoLog(shader,sizeof(error),0,error);
        else
          glGetShaderInfoLog(shader,sizeof(error),0,error);

        std::cerr << errorMessage << ": '"
                  << error
                  << "'"
                  << std::endl;
      }
  }

  static GLuint CreateShader(const std::string &text,
                             GLenum shaderType)
  {
    GLuint shader = glCreateShader(shaderType);

    if (shader == 0)
      std::cerr << "Error: Shader creation failed."
                << std::endl;

    const GLchar *shaderSourceStrings[1];
    GLint shaderSourceStringLengths[1];
    shaderSourceStrings[0]       = text.c_str();
    shaderSourceStringLengths[0] = text.length();

    glShaderSource(shader, 1, shaderSourceStrings, shaderSourceStringLengths);
    glCompileShader(shader);

    CheckShaderError(shader, GL_COMPILE_STATUS, false,
                     "Error: Shader compilation failed");

    return shader;
  }

  enum
    {
      TRANSFORM_U,

      NUM_UNIFORMS
    };

  Shader(const std::string &fileName)
  {
    program_ = glCreateProgram();

    // Vertex shader
    shaders_[0] = CreateShader(LoadShader(fileName + ".vs"),
                               GL_VERTEX_SHADER);

    // Fragment shader
    shaders_[1] = CreateShader(LoadShader(fileName + ".fs"),
                               GL_FRAGMENT_SHADER);

    for (unsigned i = 0 ; i < NUM_SHADERS; ++i)
      glAttachShader(program_,shaders_[i]);

    glBindAttribLocation(program_, 0, "position");
    glBindAttribLocation(program_, 1, "texCoord");

    glLinkProgram(program_);
    CheckShaderError(program_, GL_LINK_STATUS, true, "Error: Program linking failed");

    glValidateProgram(program_);
    CheckShaderError(program_, GL_VALIDATE_STATUS, true, "Error: Program is invalid");

    uniforms_[TRANSFORM_U] = glGetUniformLocation(program_, "transform");
  }     

  virtual ~Shader()
  {
    for (auto &shader : shaders_)
      {
        glDetachShader(program_,shader);
        glDeleteShader(shader);
      }

    glDeleteProgram(program_);
  }

  void Bind()
  {
    glUseProgram(program_);
  }

  void Update(const Transform &transform,
              const Camera    &camera)
  {
    // Parameters:
    // 1) which uniform to modify
    // 2) how many parameters we pass in
    // 3) whether to transpose
    // 4) data to pass
    // glm::mat4 model = transform.get_model();
    glm::mat4 model = camera.get_view_projection() * transform.get_model();
    glUniformMatrix4fv(uniforms_[TRANSFORM_U],1,GL_FALSE,&model[0][0]);
  }


private:
  static const unsigned NUM_SHADERS = 2;

  GLuint program_;
  GLuint shaders_[NUM_SHADERS];
  GLuint uniforms_[NUM_UNIFORMS];
};

class Texture
{
public:
  Texture(const std::string &filename)
  {
    int width, height, numComponents;
    unsigned char *imageData =
      stbi_load(filename.c_str(),
                &width, &height, &numComponents,
                4);

    if (imageData == 0)
      std::cerr << "Texture loading failed for texture '"
                << filename
                << "'"
                << std::endl;


    // Allocate space for the texture, in the GPU memory
    glGenTextures(1,&texture_);
    glBindTexture(GL_TEXTURE_2D,texture_);


    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_S,
                    GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,
                    GL_TEXTURE_WRAP_T,
                    GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D,
                    GL_TEXTURE_MAG_FILTER,
                    GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,
                 0, // which version of the texture it is (e.g. we
                    // could define differnt versions at different
                    // resolutions, to distinguish between a texture
                    // being displayed close to the camera as opposed
                    // to far from the camera),
                 GL_RGBA, // internal format used by OpenGL to store
                          // the texture data
                 width,
                 height,
                 0, // border
                 GL_RGBA, // input format (as coming from stbi)
                 GL_UNSIGNED_BYTE, // data comes as unsigned char* (from STBI)
                 imageData
                 );

    stbi_image_free(imageData);
  }

  ~Texture()
  {
    glDeleteTextures(1,&texture_);
  }

  void Bind(unsigned unit)
  {
    assert(unit <= 31);

    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D,texture_);
  }

private:
  GLuint texture_;
};

class Display
{
public:
  Display(int width, int height, const std::string &title)
    : window_(), isClosed_(false)
  {
    // How many bits of information about red, green, blue and
    // transparency components
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    // How many bits to be allocated per pixel
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);

    // Z buffer (aka depth buffer)
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    // Allocate space for a duplicate of the window (not actually
    // displayed)
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    window_ = SDL_CreateWindow(title.c_str(),
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               width,
                               height,
                               SDL_WINDOW_OPENGL);
    glContext_ = SDL_GL_CreateContext(window_);

    GLenum status = glewInit();
    if (status != GLEW_OK)
      std::cerr << "Glew failed to initialized." << std::endl;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
  }

  virtual ~Display()
  {
    SDL_GL_DeleteContext(glContext_);
    SDL_DestroyWindow(window_);
  }

  void Update()
  {
    SDL_GL_SwapWindow(window_);

    SDL_Event e;
    while (SDL_PollEvent(&e))
      {
        if (e.type == SDL_QUIT)
          isClosed_ = true;
      }
  }

  void Clear(float r, float g, float b, float a)
  {
    glClearColor(r,g,b,a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  bool isClosed() const
  { return isClosed_; }

private:
  Display(const Display &)
  { }
  Display &operator=(const Display &)
  { return *this; }

  SDL_Window    *window_;
  SDL_GLContext  glContext_;
  bool           isClosed_;
};


int main()
{

  static const float WIDTH  = 800.0f;
  static const float HEIGHT = 600.0f;

  SDL_Init(SDL_INIT_EVERYTHING);

  Display display(800,600,"Hello World");

  // Vertex vertices[] = { Vertex(glm::vec3(-0.5,-0.5,0), glm::vec2(0.0,0.0)),
  //                       Vertex(glm::vec3(0,0.5,0), glm::vec2(0.5,1.0)),
  //                       Vertex(glm::vec3(0.5,-0.5,0), glm::vec2(1.0,0.0))
  //                      };
  // 
  // unsigned int indices[] = { 0, 1, 2 };

  // Mesh mesh(vertices,
  //           sizeof(vertices) / sizeof(Vertex),
  //           indices,
  //           sizeof(indices) / sizeof(unsigned int));
  Mesh mesh("./res/glider.obj");
  std::cerr << "Loaded obj file." << std::endl;
  Shader shader("./res/basicShader");
  Texture texture("./res/bricks.jpg");
  Camera camera(glm::vec3(0,0,-40),
                70.0f, // field of view approximately like that of the human eye
                WIDTH / HEIGHT, // aspect ratio
                0.01f,
                1000.0f);
  Transform transform;
  transform.rot_.x = M_PI / 2;
  //transform.rot_.y = M_PI;

  float counter = 0.0;

  while (!display.isClosed())
    {
      display.Clear(0.0,0.15,0.3,1.0);

      // transform.pos_.x = std::sin(counter);
      // transform.pos_.z = std::sin(counter);
      // transform.scale_ = glm::vec3(counter,counter,counter);
      transform.rot_.z = counter;
      transform.rot_.x = counter;

      shader.Bind();
      texture.Bind(0);
      shader.Update(transform,camera);
      mesh.Draw();
      display.Update();
      counter += 0.01f;
      if (counter > 2*(float)M_PI)
        counter -= 2*M_PI;
    }

  SDL_Quit();
  return 0;
}
