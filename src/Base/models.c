#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "main.h"

void load_model(const char* filename, GeometryData *geometry_data) {
    printf("Loading GLB model: %s\n", filename);
    
    const struct aiScene* scene = aiImportFile(filename, 
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices | // This is CRUCIAL for indexing
        aiProcess_OptimizeMeshes |
        aiProcess_ValidateDataStructure);
    
    if (!scene || !scene->mRootNode) {
        printf("Assimp error: %s\n", aiGetErrorString());
        return;
    }
    
    printf("GLB scene loaded successfully!\n");
    printf("Number of meshes: %u\n", scene->mNumMeshes);
    
    // Count total UNIQUE vertices and indices
    size_t total_vertices = 0;
    size_t total_indices = 0;
    
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        struct aiMesh* mesh = scene->mMeshes[m];
        total_vertices += mesh->mNumVertices;    // Unique vertices per mesh
        total_indices += mesh->mNumFaces * 3;    // Indices (3 per face)
    }
    
    printf("Total unique vertices: %zu, total indices: %zu\n", total_vertices, total_indices);
    
    // Allocate memory
    geometry_data->vertices = malloc(sizeof(Vertex) * total_vertices);
    geometry_data->indices = malloc(sizeof(uint32_t) * total_indices);
    
    if (!geometry_data->vertices || !geometry_data->indices) {
        printf("Memory allocation failed!\n");
        aiReleaseImport(scene);
        return;
    }
    
    geometry_data->vertex_count = 0;
    geometry_data->index_count = 0;
    
    // Process each mesh
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        struct aiMesh* mesh = scene->mMeshes[m];
        size_t vertex_offset = geometry_data->vertex_count; // Offset for this mesh's vertices
        
        printf("Processing mesh %u: %u vertices, %u faces\n", 
               m, mesh->mNumVertices, mesh->mNumFaces);
        
        // Process UNIQUE vertices (no duplication)
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
            
            geometry_data->vertices[geometry_data->vertex_count++] = vertex;
        }
        
        // Process indices - this is where the magic happens!
        for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
            struct aiFace face = mesh->mFaces[f];
            
            if (face.mNumIndices == 3) {
                for (unsigned int i = 0; i < 3; i++) {
                    // Key difference: indices point to the unique vertices we just stored
                    // face.mIndices[i] is the index WITHIN THIS MESH
                    // vertex_offset is the starting point of this mesh's vertices in our big array
                    geometry_data->indices[geometry_data->index_count++] = face.mIndices[i] + vertex_offset;
                }
            } else {
                printf("Warning: Face with %u indices found\n", face.mNumIndices);
            }
        }
    }
    
    printf("Successfully loaded GLB model with PROPER indexing:\n");
    printf("  - Unique vertices: %zu\n", geometry_data->vertex_count);
    printf("  - Indices: %zu\n", geometry_data->index_count);
    printf("  - Vertex reuse ratio: %.1f:1\n", (float)geometry_data->index_count / geometry_data->vertex_count);
   
    aiReleaseImport(scene);
}
