#include "common.h"
#include "SourcePath.h"


using namespace Angel;

typedef vec4 color4;

// Initialize shader lighting parameters
vec4 light(   0.0, 0.0, 10.0, 1.0 );
color4 light_ambient(  0.1, 0.1, 0.1, 1.0 );
color4 light_diffuse(  1.0, 1.0, 1.0, 1.0 );
color4 light_specular( 1.0, 1.0, 1.0, 1.0 );

// Initialize shader material parameters
color4 material_ambient( 0.1, 0.1, 0.1, 1.0 );
color4 material_diffuse( 0.2, 0.6, 0.0, 1.0 );
color4 material_specular( 0.8, 0.8, 0.8, 1.0 );
float  material_shininess = 10;


enum{_IMAGE_1, _IMAGE_2, _IMAGE_3, _TOTAL_IMAGES};
std::string files[_TOTAL_IMAGES] = {"/models/three.qb",
                                    "/models/palmtree.qb",
                                    "/models/goldisle.qb",};



std::vector < VoxelGrid > voxelgrid;
std::vector < GLuint > buffer;
std::vector < GLuint > vao;
GLuint ModelView_loc, NormalMatrix_loc, Projection_loc;
bool wireframe;
int current_draw;

//==========Trackball Variables==========
static float curquat[4],lastquat[4];
/* current transformation matrix */
static float curmat[4][4];
mat4 curmat_a;
/* actual operation  */
bool scaling;
bool moving;
bool panning;
/* starting "moving" coordinates */
static int beginx, beginy;
/* ortho */
float ortho_x, ortho_y;
/* current scale factor */
static float scalefactor;
bool lbutton_down;


static void error_callback(int error, const char* description)
{
  fprintf(stderr, "Error: %s\n", description);
}

//User interaction handler
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS){
    current_draw = (current_draw+1)%_TOTAL_IMAGES;
  }
  if (key == GLFW_KEY_W && action == GLFW_PRESS){
    wireframe = !wireframe;
  }
}

//User interaction handler
static void mouse_click(GLFWwindow* window, int button, int action, int mods){
  
  if (GLFW_RELEASE == action){
    moving=scaling=panning=false;
    return;
  }
  
  if( mods & GLFW_MOD_SHIFT){
    scaling=true;
  }else if( mods & GLFW_MOD_ALT ){
    panning=true;
  }else{
    moving=true;
    trackball(lastquat, 0, 0, 0, 0);
  }
  
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);
  beginx = xpos; beginy = ypos;
}

//User interaction handler
void mouse_move(GLFWwindow* window, double x, double y){
  
  int W, H;
  glfwGetFramebufferSize(window, &W, &H);

  
  float dx=(x-beginx)/(float)W;
  float dy=(beginy-y)/(float)H;
  
  if (panning)
    {
    ortho_x  +=dx;
    ortho_y  +=dy;
    
    beginx = x; beginy = y;
    return;
    }
  else if (scaling)
    {
    scalefactor *= (1.0f+dx);
    
    beginx = x;beginy = y;
    return;
    }
  else if (moving)
    {
    trackball(lastquat,
              (2.0f * beginx - W) / W,
              (H - 2.0f * beginy) / H,
              (2.0f * x - W) / W,
              (H - 2.0f * y) / H
              );
    
    add_quats(lastquat, curquat, curquat);
    build_rotmatrix(curmat, curquat);
    
    beginx = x;beginy = y;
    return;
    }
}


void init(){
  
  std::string vshader = source_path + "/shaders/vshader.glsl";
  std::string fshader = source_path + "/shaders/fshader.glsl";
  
  GLchar* vertex_shader_source = readShaderSource(vshader.c_str());
  GLchar* fragment_shader_source = readShaderSource(fshader.c_str());

  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, (const GLchar**) &vertex_shader_source, NULL);
  glCompileShader(vertex_shader);
  check_shader_compilation(vshader, vertex_shader);
  
  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, (const GLchar**) &fragment_shader_source, NULL);
  glCompileShader(fragment_shader);
  check_shader_compilation(fshader, fragment_shader);
  
  GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  
  glLinkProgram(program);
  check_program_link(program);
  
  glUseProgram(program);
  
  glBindFragDataLocation(program, 0, "fragColor");

  //Per vertex attributes
  GLuint vPosition = glGetAttribLocation( program, "vPosition" );
  GLuint vColor = glGetAttribLocation( program, "vColor" );
  GLuint vNormal = glGetAttribLocation( program, "vNormal" );

  //Compute ambient, diffuse, and specular terms
  color4 ambient_product  = light_ambient * material_ambient;
  color4 diffuse_product  = light_diffuse * material_diffuse;
  color4 specular_product = light_specular * material_specular;
  
  //Retrieve and set uniform variables
  glUniform4fv( glGetUniformLocation(program, "Light"), 1, light);
  glUniform4fv( glGetUniformLocation(program, "AmbientProduct"), 1, ambient_product );
  glUniform4fv( glGetUniformLocation(program, "DiffuseProduct"), 1, diffuse_product );
  glUniform4fv( glGetUniformLocation(program, "SpecularProduct"), 1, specular_product );
  glUniform1f(  glGetUniformLocation(program, "Shininess"), material_shininess );
  
  //Matrix uniform variable locations
  ModelView_loc = glGetUniformLocation( program, "ModelView" );
  NormalMatrix_loc = glGetUniformLocation( program, "NormalMatrix" );
  Projection_loc = glGetUniformLocation( program, "Projection" );
  
  //===== Send data to GPU ======
  vao.resize(_TOTAL_IMAGES);
  glGenVertexArrays( _TOTAL_IMAGES, &vao[0] );
  
  buffer.resize(_TOTAL_IMAGES);
  glGenBuffers( _TOTAL_IMAGES, &buffer[0] );
  
  for(unsigned int i=0; i < _TOTAL_IMAGES; i++){
    voxelgrid.push_back((source_path + files[i]).c_str());

    // match normal array size of vertices
    if (voxelgrid[i].normals.size() < voxelgrid[i].vertices.size()) {
      std::size_t oldsize = voxelgrid[i].normals.size();
      std::size_t newsize = voxelgrid[i].vertices.size();
      voxelgrid[i].normals.resize(newsize);
      for (std::size_t j = oldsize; j < newsize; ++j) {
        voxelgrid[i].normals[j] = vec3(0.f, -1.f, 0.f);
      }
    }
    if (voxelgrid[i].colors.size() < voxelgrid[i].vertices.size()) {
      std::size_t oldsize = voxelgrid[i].colors.size();
      std::size_t newsize = voxelgrid[i].vertices.size();
      voxelgrid[i].colors.resize(newsize);
      for (std::size_t j = oldsize; j < newsize; ++j) {
        voxelgrid[i].colors[j] = vec3(0.f, 0.f, 0.f);
      }
    }

    glBindVertexArray( vao[i] );
    glBindBuffer( GL_ARRAY_BUFFER, buffer[i] );
    unsigned int vertices_bytes = voxelgrid[i].vertices.size()*sizeof(vec4);
    unsigned int colors_bytes  = voxelgrid[i].colors.size()*sizeof(vec3);
    unsigned int normals_bytes  = voxelgrid[i].normals.size()*sizeof(vec3);
    
    glBufferData( GL_ARRAY_BUFFER, vertices_bytes + colors_bytes + normals_bytes, NULL, GL_STATIC_DRAW );
    unsigned int offset = 0;
    if (vertices_bytes > 0) {
      glBufferSubData( GL_ARRAY_BUFFER, offset, vertices_bytes, &voxelgrid[i].vertices[0] );
    }
    offset += vertices_bytes;
    if (colors_bytes > 0) {
      glBufferSubData( GL_ARRAY_BUFFER, offset, colors_bytes,  &voxelgrid[i].colors[0] );
    }
    offset += colors_bytes;
    if (normals_bytes > 0) {
      glBufferSubData( GL_ARRAY_BUFFER, offset, normals_bytes,  &voxelgrid[i].normals[0] );
    }
    
    glEnableVertexAttribArray( vColor );
    glEnableVertexAttribArray( vPosition );
    glEnableVertexAttribArray( vNormal );

    if (vertices_bytes > 0)
      glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );
    if (colors_bytes > 0)
      glVertexAttribPointer( vColor, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(static_cast<size_t>(vertices_bytes)) );
    if (normals_bytes > 0)
      glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(static_cast<size_t>(vertices_bytes + colors_bytes)) );

  }
  
  //===== End: Send data to GPU ======


  // ====== Enable some opengl capabilitions ======
  glEnable( GL_DEPTH_TEST );
  glShadeModel(GL_SMOOTH);

  glClearColor( 0.8, 0.8, 1.0, 1.0 );
  
  //===== Initalize some program state variables ======

  //Quaternion trackball variables, you can ignore
  scaling  = 0;
  moving   = 0;
  panning  = 0;
  beginx   = 0;
  beginy   = 0;
  
  matident(curmat);
  trackball(curquat , 0.0f, 0.0f, 0.0f, 0.0f);
  trackball(lastquat, 0.0f, 0.0f, 0.0f, 0.0f);
  add_quats(lastquat, curquat, curquat);
  build_rotmatrix(curmat, curquat);
  
  scalefactor = 1.0;
  
  wireframe = false;
  current_draw = 0;
  
  lbutton_down = false;


  //===== End: Initalize some program state variables ======

}


int main(void){
  
  GLFWwindow* window;
  
  glfwSetErrorCallback(error_callback);
  
  if (!glfwInit())
    exit(EXIT_FAILURE);
  
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  
  glfwWindowHint(GLFW_SAMPLES, 4);
  
  window = glfwCreateWindow(512, 512, "Assignment 5 - Volumetric Models", NULL, NULL);
  if (!window){
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  
  //Set key and mouse callback functions
  glfwSetKeyCallback(window, key_callback);
  glfwSetMouseButtonCallback(window, mouse_click);
  glfwSetCursorPosCallback(window, mouse_move);

  
  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
  glfwSwapInterval(1);
  
  init();
  

  
  while (!glfwWindowShouldClose(window)){
    
    //Display as wirfram, boolean tied to keystoke 'w'
    if(wireframe){
      glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    }else{
      glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    }
    
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    
    GLfloat aspect = GLfloat(width)/height;
    
    //Projection matrix
    mat4  projection = Perspective( 45.0, aspect, 0.5, 5.0 );
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //"Camera" position
    const vec3 viewer_pos( 0.0, 0.0, 2.0 );
    
    //Track_ball rotation matrix
    mat4 track_ball =  mat4(curmat[0][0], curmat[1][0], curmat[2][0], curmat[3][0],
                            curmat[0][1], curmat[1][1], curmat[2][1], curmat[3][1],
                            curmat[0][2], curmat[1][2], curmat[2][2], curmat[3][2],
                            curmat[0][3], curmat[1][3], curmat[2][3], curmat[3][3]);
 
    //Modelview based on user interaction
    mat4 user_MV  =  Translate( -viewer_pos ) *                    //Move Camera Back to -viewer_pos
                     Translate(ortho_x, ortho_y, 0.0) *            //Pan Camera
                     track_ball *                                  //Rotate Camera
                     Scale(scalefactor,scalefactor,scalefactor);   //User Scale
    

    // ====== Draw ======
    glBindVertexArray(vao[current_draw]);
    //glBindBuffer( GL_ARRAY_BUFFER, buffer[current_draw] );
    
    glUniformMatrix4fv( ModelView_loc, 1, GL_TRUE, user_MV*voxelgrid[current_draw].model_view);
    glUniformMatrix4fv( Projection_loc, 1, GL_TRUE, projection );
    glUniformMatrix4fv( NormalMatrix_loc, 1, GL_TRUE, transpose(invert(user_MV*voxelgrid[current_draw].model_view)));

    glDrawArrays( GL_TRIANGLES, 0, voxelgrid[current_draw].vertices.size() );
    // ====== End: Draw ======

    
    glfwSwapBuffers(window);
    glfwPollEvents();
    
  }
  
  glfwDestroyWindow(window);
  
  glfwTerminate();
  exit(EXIT_SUCCESS);
}
