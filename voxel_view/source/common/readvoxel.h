#ifndef hg_READVOXEL_h_
#define hg_READVOXEL_h_

#include <vector>

/**
 * @brief Convert an error code to a string.
 * @param[out] voxels buffer to hold voxel data
 * @param[out] width x-axis size of voxel grid
 * @param[out] height y-axis size of voxel grid
 * @param[out] depth z-axis size of voxel grid
 * @param path path to `.qb` file to load
 * @return zero on success, nonzero error code otherwise
 */
unsigned int voxelgrid_decode(std::vector<unsigned char>& voxels,
  unsigned int& width, unsigned int& height, unsigned int &depth,
  const char* path);

/**
 * @brief Convert an error code to a string.
 * @param err code
 * @return corresponding string
 */
const char* voxelgrid_error_text(unsigned int err);

#endif //hg_READVOXEL_h_
