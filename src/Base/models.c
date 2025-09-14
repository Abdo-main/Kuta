#include "main.h"
#include "vertex_data.h"
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

void load_models(const char *filenames[], Models *models, size_t count,
                 BufferData *buffer_data, State *state) {
  for (size_t i = 0; i < count; i++) {
    printf("Loading GLB model: %s\n", filenames[i]);

    const struct aiScene *scene =
        aiImportFile(filenames[i], aiProcess_Triangulate | aiProcess_FlipUVs |
                                       aiProcess_GenNormals |
                                       aiProcess_JoinIdenticalVertices |
                                       aiProcess_OptimizeMeshes |
                                       aiProcess_ValidateDataStructure);

    if (!scene || !scene->mRootNode) {
      printf("Assimp error: %s\n", aiGetErrorString());
      return;
    }

    // Count total vertices and indices
    models->total_vertices = 0;
    models->total_indices = 0;

    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
      struct aiMesh *mesh = scene->mMeshes[m];
      models->total_vertices += mesh->mNumVertices;
      models->total_indices += mesh->mNumFaces * 3;
    }
  }

  for (size_t i = 0; i < count; i++) {
    const struct aiScene *scene =
        aiImportFile(filenames[i], aiProcess_Triangulate | aiProcess_FlipUVs |
                                       aiProcess_GenNormals |
                                       aiProcess_JoinIdenticalVertices |
                                       aiProcess_OptimizeMeshes |
                                       aiProcess_ValidateDataStructure);

    if (!scene || !scene->mRootNode) {
      printf("Assimp error: %s\n", aiGetErrorString());
      return;
    }

    printf("GLB scene loaded successfully!\n");
    printf("Number of meshes: %u\n", scene->mNumMeshes);

    // Allocate memory for vertices and indices
    models->geometry[i].vertices =
        malloc(sizeof(Vertex) * models->total_vertices);
    models->geometry[i].indices =
        malloc(sizeof(uint32_t) * models->total_indices);

    if (!models->geometry[i].vertices || !models->geometry[i].indices) {
      printf("Memory allocation failed!\n");
      free(&models->geometry[i]);
      return;
    }

    // INITIALIZE COUNTERS!
    models->geometry[i].vertex_count = 0;
    models->geometry[i].index_count = 0;

    // Process each mesh
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
      struct aiMesh *mesh = scene->mMeshes[m];
      size_t vertex_offset =
          models->geometry[i].vertex_count; // Use the actual counter

      printf("Processing mesh %u: %u vertices, %u faces\n", m,
             mesh->mNumVertices, mesh->mNumFaces);

      // Process vertices
      for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
        Vertex vertex = {0};

        // Position
        vertex.pos[0] = mesh->mVertices[v].x;
        vertex.pos[1] = mesh->mVertices[v].y;
        vertex.pos[2] = mesh->mVertices[v].z;

        // Texture coordinates
        if (mesh->mTextureCoords[0]) {
          vertex.tex_coord[0] = mesh->mTextureCoords[0][v].x;
          vertex.tex_coord[1] = mesh->mTextureCoords[0][v].y;
        } else {
          vertex.tex_coord[0] = 0.0f;
          vertex.tex_coord[1] = 0.0f;
        }

        // Vertex colors
        if (mesh->mColors[0]) {
          vertex.color[0] = mesh->mColors[0][v].r;
          vertex.color[1] = mesh->mColors[0][v].g;
          vertex.color[2] = mesh->mColors[0][v].b;
        } else {
          vertex.color[0] = 1.0f;
          vertex.color[1] = 1.0f;
          vertex.color[2] = 1.0f;
        }

        models->geometry[i].vertices[models->geometry[i].vertex_count++] =
            vertex;
      }

      // Process indices
      for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
        struct aiFace face = mesh->mFaces[f];

        if (face.mNumIndices == 3) {
          for (unsigned int d = 0; d < 3; d++) {
            models->geometry[i].indices[models->geometry[i].index_count++] =
                face.mIndices[d] + vertex_offset;
          }
        } else {
          printf("Warning: Face with %u indices found\n", face.mNumIndices);
        }
      }
    }

    printf("Successfully loaded GLB model:\n");
    printf("  - Vertices: %zu\n", models->geometry[i].vertex_count);
    printf("  - Indices: %zu\n", models->geometry[i].index_count);
    printf("  - Vertex reuse ratio: %.1f:1\n",
           (float)models->geometry[i].index_count /
               models->geometry[i].vertex_count);

    create_vertex_buffer(buffer_data, models, state, i);
    create_index_buffer(buffer_data, models, state, i);

    aiReleaseImport(scene);
  }
}
