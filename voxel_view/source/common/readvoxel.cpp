
#include "readvoxel.h"
#include "u8names.h"
#include <qbvoxel/parse.h>
#include <cstdio>
#include <cerrno>
#include <limits>
#include <new>

struct voxelgrid_cb_data {
  std::vector<unsigned char>* image;
  unsigned int width;
  unsigned int height;
  unsigned int depth;
};
static
int voxelgrid_cb_resize(void* p, unsigned long int n);
static
int voxelgrid_cb_set_matrix
  (void* p, unsigned long int i, const qbvoxel_matrix_info* mi);
static
int voxelgrid_cb_write_voxel
  ( void* p, unsigned long int i,
    unsigned long int x,unsigned long int y,unsigned long int z,
    const qbvoxel_voxel* v);


int voxelgrid_cb_resize(void* , unsigned long int n) {
  return n == 1 ? QBVoxel_Ok : /*matrix count*/10;
}

int voxelgrid_cb_set_matrix
  (void* p, unsigned long int i, const qbvoxel_matrix_info* mi)
{
  voxelgrid_cb_data* data = static_cast<voxelgrid_cb_data*>(p);
  const size_t max_voxels = (std::numeric_limits<size_t>::max()/4);
  size_t space_needed = 0;
  if (i != 0)
    return QBVoxel_ErrOutOfRange;
  else if (mi->size_x == 0 || mi->size_y == 0 || mi->size_z == 0) {
    space_needed = 0;
  } else if (mi->size_x > max_voxels){
    return QBVoxel_ErrMemory;
  } else if (mi->size_y > max_voxels/mi->size_x){
    return QBVoxel_ErrMemory;
  } else if (mi->size_z > (max_voxels/mi->size_x)/mi->size_y){
    return QBVoxel_ErrMemory;
  } else {
    space_needed = static_cast<size_t>(mi->size_x)
        * static_cast<size_t>(mi->size_y)
        * static_cast<size_t>(mi->size_z) * 4;
  }
  try {
    data->image->resize(space_needed);
  } catch (const std::bad_alloc& ){
    return QBVoxel_ErrMemory;
  }
  data->width = static_cast<unsigned int>(mi->size_x);
  data->height = static_cast<unsigned int>(mi->size_y);
  data->depth = static_cast<unsigned int>(mi->size_z);
  return QBVoxel_Ok;
}

int voxelgrid_cb_write_voxel
  ( void* p, unsigned long int i,
    unsigned long int x,unsigned long int y,unsigned long int z,
    const qbvoxel_voxel* v)
{
  voxelgrid_cb_data* data = static_cast<voxelgrid_cb_data*>(p);
  if (i != 0)
    return QBVoxel_ErrOutOfRange;
  else if (x >= data->width)
    return QBVoxel_ErrOutOfRange;
  else if (y >= data->height)
    return QBVoxel_ErrOutOfRange;
  else if (z >= data->depth)
    return QBVoxel_ErrOutOfRange;
  else {
    size_t pos = x + (y + static_cast<size_t>(z)*data->height) * data->width;
    std::vector<unsigned char>& voxels = (*data->image);
    voxels[pos*4] = v->r;
    voxels[pos*4+1] = v->g;
    voxels[pos*4+2] = v->b;
    voxels[pos*4+3] = v->a;
    return QBVoxel_Ok;
  }
}


unsigned int
voxelgrid_decode(std::vector<unsigned char>& image,
  unsigned int& width, unsigned int& height, unsigned int &depth,
  const char* path)
{
#ifdef _WIN32
  std::FILE* fp ;
  /* */{
    std::wstring wcpath;
    if (u8names_towc(path, wcpath) != 0)
      return 9/* other io */;
    fp = _wfopen(wcpath.c_str(), L"rb");
  }
#else
  std::FILE* fp = std::fopen(path, "rb");
#endif //_WIN32
  if (fp != NULL) {
    unsigned int error_code = 0;
    unsigned char buf[256];
    std::size_t readsize;
    voxelgrid_cb_data data = {&image, 0,0,0};
    qbvoxel_i cb = {&data, voxelgrid_cb_resize, NULL, NULL,
      &voxelgrid_cb_set_matrix, NULL, &voxelgrid_cb_write_voxel };
    qbvoxel_state state = {0};
    qbvoxel_parse_init(&state, &cb);
    while ((readsize = std::fread(buf, 1, 256, fp)) > 0) {
      unsigned int len =
        qbvoxel_parse_do(&state, static_cast<unsigned int>(readsize), buf);
      if (len < readsize)
        break;
    }
    error_code = qbvoxel_api_get_error(&state);
    qbvoxel_parse_clear(&state);
    std::fclose(fp);
    width = data.width;
    height = data.height;
    depth = data.depth;
    return error_code;
  } else {
#ifdef ENOENT
    return errno == ENOENT ? 8/* file not found */ : 9/* other io */;
#else
    return 9/* other io */;
#endif //ENOENT
  }
}

char const* voxelgrid_error_text(unsigned err) {
  switch (err) {
  case QBVoxel_Ok: return "Success";
  case QBVoxel_ErrVersion: return "Unsupported version";
  case QBVoxel_ErrColorType: return "Unsupported color type";
  case QBVoxel_ErrOrientation: return "Unsupported orientation";
  case QBVoxel_ErrMaskFormat: return "Unsupported mask format";
  case QBVoxel_ErrCompress: return "Unsupported compression method";
  case QBVoxel_ErrOutOfRange: return "Out of range";
  case QBVoxel_ErrMemory: return "Out of memory";
  case 8: return "File not found";
  case 9: return "Input/output error";
  case 10: return "Unsupported matrix count";
  default: return "Unknown error";
  }
}
