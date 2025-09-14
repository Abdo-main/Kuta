#include "main.h"
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

GeometryData load_models(const char *filename) {
  GeometryData geometry = {0};
  const struct aiScene *scene = aiImportFile(
      filename, aiProcess_Triangulate | aiProcess_FlipUVs |
                    aiProcess_GenNormals | aiProcess_JoinIdenticalVertices |
                    aiProcess_OptimizeMeshes | aiProcess_ValidateDataStructure);
  if (!scene || !scene->mRootNode) {

    printf("Assimp error: %s\n", aiGetErrorString());
    return geometry;
  }

  // Count total vertices & indices
  geometry.vertex_count = 0;
  geometry.index_count = 0;
  size_t total_vertices = 0, total_indices = 0;

  for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
    struct aiMesh *mesh = scene->mMeshes[m];
    total_vertices += mesh->mNumVertices;
    total_indices += mesh->mNumFaces * 3;
  }

  // Allocate arrays
  geometry.vertices = malloc(sizeof(Vertex) * total_vertices);
  geometry.indices = malloc(sizeof(uint32_t) * total_indices);
  geometry.vertex_count = 0;
  geometry.index_count = 0;

  // Fill geometry.vertices and geometry.indices
  for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
    struct aiMesh *mesh = scene->mMeshes[m];
    size_t vertex_offset = geometry.vertex_count;

    // Copy vertices
    for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
      Vertex vert = {0};
      vert.pos[0] = mesh->mVertices[v].x;
      vert.pos[1] = mesh->mVertices[v].y;
      vert.pos[2] = mesh->mVertices[v].z;
      if (mesh->mTextureCoords[0]) {
        vert.tex_coord[0] = mesh->mTextureCoords[0][v].x;
        vert.tex_coord[1] = mesh->mTextureCoords[0][v].y;
      }
      geometry.vertices[geometry.vertex_count++] = vert;
    }

    // Copy indices
    for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
      struct aiFace face = mesh->mFaces[f];
      if (face.mNumIndices == 3) {
        for (unsigned int d = 0; d < 3; d++) {
          geometry.indices[geometry.index_count++] =
              face.mIndices[d] + vertex_offset;
        }
      }
    }
  }

  aiReleaseImport(scene);
  return geometry;
}
