

#ifndef __VOXELGRID__
#define __VOXELGRID__

#include "common.h"

using namespace Angel;

class VoxelGrid{
public:
  unsigned int width, height, depth;
  std::vector<unsigned char> volume;
  
  std::vector < vec4 > vertices;
  std::vector < vec3 > normals;
  std::vector < vec3 > colors;
  
  mat4 model_view;
  
  VoxelGrid(const char * path) : model_view(){
    if(loadVoxels(path)){
      createMesh();
      createNormals();
      createColors();
    }
  }
  
  unsigned int getNumTri(){ return vertices.size()/3; }

  bool loadVoxels(const char * path);
  
  void addCube(vec3 pos);
  void createMesh();
  void createNormals();
  void createColors();

  
  friend std::ostream& operator << ( std::ostream& os, const VoxelGrid& v ) {
    os << "Vertices:\n";
    for(unsigned int i=0; i < v.vertices.size(); i++){
      os << "\t\t" << v.vertices[i] << "\n";
    }
    os << "Normals:\n";
    for(unsigned int i=0; i < v.normals.size(); i++){
      os << "\t\t" << v.normals[i] << "\n";
    }
    os << "Colors:\n";
    for(unsigned int i=0; i < v.colors.size(); i++){
      os << "\t\t" << v.colors[i] << "\n";
    }
 
    return os;
  }
  
};


#endif  //#ifndef __OBJLOADER__
