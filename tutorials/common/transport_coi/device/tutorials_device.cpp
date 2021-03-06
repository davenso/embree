// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "tutorial/obj_loader.h"
#include "transport/transport_host.h"
#include "transport/transport_device.h"
#include "transport_coi/common.h"

#include <sink/COIPipeline_sink.h>
#include <sink/COIProcess_sink.h>
#include <common/COIMacros_common.h>
#include <common/COISysInfo_common.h>
#include <common/COIEvent_common.h>


extern "C" int64 get_tsc() {
  return read_tsc();
}

namespace embree
{
  /* ISPC compatible mesh */
  struct ISPCMesh
  {
    ALIGNED_CLASS;
  public:
    ISPCMesh (int numTriangles, 
	      int numQuads, 
	      int numVertices, 
	      int meshMaterialID) 
      : numTriangles(numTriangles), numQuads(numQuads), numVertices(numVertices),
        positions(NULL), positions2(NULL), normals(NULL), texcoords(NULL), triangles(NULL), quads(NULL), edge_level(NULL), meshMaterialID(meshMaterialID)
    {
      sizePositions = 0;
      sizeNormals   = 0;
      sizeTexCoords = 0;
      sizeTriangles = 0;
      sizeQuads     = 0;
    }

    ~ISPCMesh () {
      if (positions)  os_free(positions ,sizePositions);
      if (positions2) os_free(positions2,sizePositions);
      if (normals)    os_free(normals   ,sizeNormals);
      if (texcoords)  os_free(texcoords ,sizeTexCoords);
      if (triangles)  os_free(triangles ,sizeTriangles);
      if (quads)      os_free(quads     ,sizeQuads);

      positions = NULL;
      positions2 = NULL;
      normals   = NULL;
      texcoords = NULL;
      triangles = NULL;
      quads = NULL;
    }

  public:
    Vec3fa* positions;     //!< vertex position array
    Vec3fa* positions2;    //!< vertex position array
    Vec3fa* normals;       //!< vertex normal array
    Vec2f*  texcoords;     //!< vertex texcoord array
    OBJScene::Triangle* triangles;  //!< list of triangles
    OBJScene::Quad* quads;          //!< list of quads
    float *edge_level;

    int numVertices;
    int numTriangles;
    int numQuads;
    int geomID;
    int meshMaterialID;
    size_t sizePositions;
    size_t sizeNormals;
    size_t sizeTexCoords;
    size_t sizeTriangles;
    size_t sizeQuads;
  };


struct ISPCSubdivMesh
{
  ALIGNED_CLASS;
public:
  ISPCSubdivMesh(int numVertices, int numFaces, int numEdges, int materialID) : numVertices(numVertices), numFaces(numFaces), numEdges(numEdges), materialID(materialID), 
										positions(NULL),normals(NULL),position_indices(NULL),
								normal_indices(NULL),texcoord_indices(NULL), verticesPerFace(NULL),
								holes(NULL), subdivlevel(NULL), 
								edge_creases(NULL), edge_crease_weights(NULL), vertex_creases(NULL), 
								vertex_crease_weights(NULL)
  {
    //DBG_PRINT(numVertices);
    //DBG_PRINT(numFaces);
    //DBG_PRINT(numEdges);
  }

  Vec3fa* positions;       //!< vertex positions
  Vec3fa* normals;         //!< face vertex normals
  Vec2f* texcoords;        //!< face texture coordinates
  int* position_indices;   //!< position indices for all faces
  int* normal_indices;     //!< normal indices for all faces
  int* texcoord_indices;   //!< texcoord indices for all faces
  int* verticesPerFace;    //!< number of indices of each face
  int* holes;              //!< face ID of holes
  float* subdivlevel;      //!< subdivision level
  Vec2i* edge_creases;          //!< crease index pairs
  float* edge_crease_weights;   //!< weight for each crease
  int* vertex_creases;          //!< indices of vertex creases
  float* vertex_crease_weights; //!< weight for each vertex crease
  int* face_offsets;
  int numVertices;
  int numFaces;
  int numEdges;
  int numEdgeCreases;
  int numVertexCreases;
  int numHoles;
  int materialID;
  int geomID;
};

  /* ISPC compatible scene */
  struct ISPCHair
  {
  public:
    ISPCHair () {}
    ISPCHair (int vertex, int id)
      : vertex(vertex), id(id) {}

    int vertex,id;  //!< index of first control point and hair ID
  };

  /*! Hair Set. */
  struct ISPCHairSet
  {
    ALIGNED_CLASS;
  public:
    Vec3fa *positions;   //!< hair control points (x,y,z,r)
    Vec3fa *positions2;   //!< hair control points (x,y,z,r)
    ISPCHair *hairs;    //!< list of hairs
    int numVertices;
    int numHairs;
    ISPCHairSet(int numHairs, int numVertices) 
      : numHairs(numHairs),numVertices(numVertices),positions(NULL),positions2(NULL),hairs(NULL) {}
    ~ISPCHairSet() {
      if (positions) free(positions);
      if (positions2) free(positions2);
      if (hairs) free(hairs);
    }
  };

  /* ISPC compatible scene */
  struct ISPCScene
  {
    ALIGNED_CLASS;
  public:
    ISPCScene (int numMeshes, int numHairSets, 
               void* materials_in, int numMaterials,
               void* ambientLights_in, int numAmbientLights,
               void* pointLights_in, int numPointLights,
               void* directionalLights_in, int numDirectionalLights,
               void* distantLights_in, int numDistantLights,
	       int numSubdivMeshes)

      : meshes(NULL), numMeshes(numMeshes), numHairSets(numHairSets), 
        materials(NULL), numMaterials(numMaterials),
        ambientLights(NULL), numAmbientLights(numAmbientLights),
        pointLights(NULL), numPointLights(numPointLights),
        directionalLights(NULL), numDirectionalLights(numDirectionalLights),
        distantLights(NULL), numDistantLights(numDistantLights),
	subdiv(NULL), numSubdivMeshes(numSubdivMeshes)
      {
        meshes = new ISPCMesh*[numMeshes];
        for (size_t i=0; i<numMeshes; i++)
          meshes[i] = NULL;

        hairsets = new ISPCHairSet*[numHairSets];
        for (size_t i=0; i<numHairSets; i++)
          hairsets[i] = NULL;
        
        materials = new OBJScene::Material[numMaterials];
        memcpy(materials,materials_in,numMaterials*sizeof(OBJScene::Material));

        ambientLights = new OBJScene::AmbientLight[numAmbientLights];
        memcpy(ambientLights,ambientLights_in,numAmbientLights*sizeof(OBJScene::AmbientLight));

        pointLights = new OBJScene::PointLight[numPointLights];
        memcpy(pointLights,pointLights_in,numPointLights*sizeof(OBJScene::PointLight));

        directionalLights = new OBJScene::DirectionalLight[numDirectionalLights];
        memcpy(directionalLights,directionalLights_in,numDirectionalLights*sizeof(OBJScene::DirectionalLight));

        distantLights = new OBJScene::DistantLight[numDistantLights];
        memcpy(distantLights,distantLights_in,numDistantLights*sizeof(OBJScene::DistantLight));

        subdiv = new ISPCSubdivMesh*[numSubdivMeshes];
        for (size_t i=0; i<numSubdivMeshes; i++)
          subdiv[i] = NULL;
      }

    ~ISPCScene () 
    {
      delete[] materials;
      delete[] ambientLights;
      delete[] pointLights;
      delete[] directionalLights;
      delete[] distantLights;

      if (meshes) {
        for (size_t i=0; i<numMeshes; i++)
          if (meshes[i]) delete meshes[i];
	delete[] meshes;
	meshes = NULL;
      }
    }

  public:
    ISPCMesh** meshes;
    OBJScene::Material* materials;  //!< material list
    int numMeshes;
    int numMaterials;

    ISPCHairSet** hairsets;
    int numHairSets;

    OBJScene::AmbientLight* ambientLights;
    int numAmbientLights;

    OBJScene::PointLight* pointLights;
    int numPointLights;

    OBJScene::DirectionalLight* directionalLights;
    int numDirectionalLights;

    OBJScene::DistantLight* distantLights;
    int numDistantLights;

    ISPCSubdivMesh** subdiv;
    int numSubdivMeshes; 
  };

  /* scene */
  static size_t g_meshID = 0;
  static size_t g_hairsetID = 0;

  extern "C" ISPCScene* g_ispc_scene = NULL;

  extern "C" void run_init(uint32_t         in_BufferCount,
                           void**           in_ppBufferPointers,
                           uint64_t*        in_pBufferLengths,
                           InitData*        in_pMiscData,
                           uint16_t         in_MiscDataLength,
                           void*            in_pReturnValue,
                           uint16_t         in_ReturnValueLength)
  {
    device_init(in_pMiscData->cfg);
  }

  extern "C" void run_key_pressed(uint32_t         in_BufferCount,
                                  void**           in_ppBufferPointers,
                                  uint64_t*        in_pBufferLengths,
                                  KeyPressedData* in_pMiscData,
                                  uint16_t         in_MiscDataLength,
                                  void*            in_pReturnValue,
                                  uint16_t         in_ReturnValueLength)
  {
    device_key_pressed(in_pMiscData->key);
  }

  extern "C" void run_create_mesh(uint32_t         in_BufferCount,
                                  void**           in_ppBufferPointers,
                                  uint64_t*        in_pBufferLengths,
                                  CreateMeshData*  in_pMiscData,
                                  uint16_t         in_MiscDataLength,
                                  void*            in_pReturnValue,
                                  uint16_t         in_ReturnValueLength)
  {
    size_t meshID = g_meshID++;

#if 0
    DBG_PRINT( in_pMiscData->numTriangles );
    DBG_PRINT( in_pMiscData->numQuads );
    DBG_PRINT( in_pMiscData->numVertices );
#endif

    ISPCMesh* mesh = new ISPCMesh(in_pMiscData->numTriangles,in_pMiscData->numQuads,in_pMiscData->numVertices,in_pMiscData->meshMaterialID);
    assert( mesh );
    assert( in_pMiscData->numTriangles*sizeof(OBJScene::Triangle) == in_pBufferLengths[3] );
    assert( in_pMiscData->numQuads*sizeof(OBJScene::Quad) == in_pBufferLengths[4] );

    //assert( in_pMiscData->numVertices*sizeof(Vec3fa) == in_pBufferLengths[1] );

    mesh->positions = (Vec3fa*)os_malloc(in_pBufferLengths[0]);
    mesh->normals   = (Vec3fa*)os_malloc(in_pBufferLengths[1]);
    mesh->texcoords = (Vec2f* )os_malloc(in_pBufferLengths[2]);
    mesh->triangles = (OBJScene::Triangle*)os_malloc(in_pBufferLengths[3]);
    mesh->quads     = (OBJScene::Quad*)os_malloc(in_pBufferLengths[4]);

    memcpy(mesh->positions,in_ppBufferPointers[0],in_pBufferLengths[0]);
    memcpy(mesh->normals  ,in_ppBufferPointers[1],in_pBufferLengths[1]);
    memcpy(mesh->texcoords,in_ppBufferPointers[2],in_pBufferLengths[2]);
    memcpy(mesh->triangles,in_ppBufferPointers[3],in_pBufferLengths[3]);
    memcpy(mesh->quads    ,in_ppBufferPointers[4],in_pBufferLengths[4]);

    mesh->sizePositions = in_pBufferLengths[0];
    mesh->sizeNormals   = in_pBufferLengths[1];
    mesh->sizeTexCoords = in_pBufferLengths[2];
    mesh->sizeTriangles = in_pBufferLengths[3];
    mesh->sizeQuads     = in_pBufferLengths[4];
    
#if 1
    if (mesh->quads[0].v0 == 0,
	mesh->quads[0].v1 == 0,
	mesh->quads[0].v2 == 0,
	mesh->quads[0].v3 == 0)
      {
	mesh->quads = NULL;
	mesh->numQuads = 0;
	mesh->sizeQuads = 0;
      }
#endif

#if 0
    DBG_PRINT( mesh->sizePositions );
    DBG_PRINT( mesh->sizeNormals );
    DBG_PRINT( mesh->sizeTexCoords );
    DBG_PRINT( mesh->sizeTriangles );
    DBG_PRINT( mesh->sizeQuads );
#endif

    g_ispc_scene->meshes[meshID] = mesh;
  }


  extern "C" void run_create_subdiv_mesh(uint32_t         in_BufferCount,
					 void**           in_ppBufferPointers,
					 uint64_t*        in_pBufferLengths,
					 CreateSubdivMeshData*  in_pMiscData,
					 uint16_t         in_MiscDataLength,
					 void*            in_pReturnValue,
					 uint16_t         in_ReturnValueLength)
  {
    size_t meshID = g_meshID++;

    const size_t numVertices = in_pMiscData->numPositions;
    const size_t numEdges    = in_pMiscData->numPositionIndices;
    const size_t numFaces    = in_pMiscData->numVerticesPerFace;

#if 0
    DBG_PRINT( numVertices );
    DBG_PRINT( numEdges );
    DBG_PRINT( numFaces );
#endif

    ISPCSubdivMesh* mesh = new ISPCSubdivMesh(in_pMiscData->numPositions,
					      in_pMiscData->numVerticesPerFace,
					      in_pMiscData->numPositionIndices,
					      in_pMiscData->materialID);
    assert( mesh );
       
    assert( in_pMiscData->numPositions*sizeof(Vec3fa)    == in_pBufferLengths[0] );
    assert( in_pMiscData->numPositionIndices*sizeof(int) == in_pBufferLengths[1] );
    assert( in_pMiscData->numVerticesPerFace*sizeof(int) == in_pBufferLengths[2] );

    mesh->positions        = (Vec3fa*)os_malloc(in_pBufferLengths[0]);
    memcpy(mesh->positions       ,in_ppBufferPointers[0],in_pBufferLengths[0]);

    mesh->position_indices = (int*)   os_malloc(in_pBufferLengths[1]);
    memcpy(mesh->position_indices,in_ppBufferPointers[1],in_pBufferLengths[1]);

    mesh->verticesPerFace  = (int*)   os_malloc(in_pBufferLengths[2]);
    memcpy(mesh->verticesPerFace ,in_ppBufferPointers[2],in_pBufferLengths[2]);


    mesh->subdivlevel      = (float*) os_malloc(in_pBufferLengths[1]);
    mesh->face_offsets     = (int*)   os_malloc(sizeof(int) * in_pMiscData->numVerticesPerFace);


    if ( in_pMiscData->numEdgeCreases )
      {
	assert(in_pBufferLengths[3] == sizeof(Vec2i) * in_pMiscData->numEdgeCreases);
	mesh->edge_creases = (Vec2i*)os_malloc(sizeof(Vec2i) * in_pMiscData->numEdgeCreases); 
	memcpy(mesh->edge_creases ,in_ppBufferPointers[3],in_pBufferLengths[3]);	
	mesh->numEdgeCreases = in_pMiscData->numEdgeCreases;
      }

    if ( in_pMiscData->numEdgeCreaseWeights )
      {
	assert(in_pBufferLengths[4] == sizeof(float) * in_pMiscData->numEdgeCreaseWeights);
	mesh->edge_crease_weights = (float*)os_malloc(sizeof(float) * in_pMiscData->numEdgeCreaseWeights); 
	memcpy(mesh->edge_crease_weights ,in_ppBufferPointers[4],in_pBufferLengths[4]);	
      }

    if ( in_pMiscData->numVertexCreases )
      {
	mesh->numVertexCreases = in_pMiscData->numVertexCreases;
	assert(in_pBufferLengths[5] == sizeof(int) * in_pMiscData->numVertexCreases);
	mesh->vertex_creases = (int*)os_malloc(sizeof(int) *  in_pMiscData->numVertexCreases); 
	memcpy(mesh->vertex_creases ,in_ppBufferPointers[5],in_pBufferLengths[5]);	
      }

    if ( in_pMiscData->numVertexCreaseWeights )
      {
	assert(in_pBufferLengths[6] == sizeof(float) * in_pMiscData->numVertexCreaseWeights);
	mesh->vertex_crease_weights = (float*)os_malloc(sizeof(float) * in_pMiscData->numVertexCreaseWeights); 
	memcpy(mesh->vertex_crease_weights ,in_ppBufferPointers[6],in_pBufferLengths[6]);	
      }

    if ( in_pMiscData->numHoles )
      {
	mesh->numHoles = in_pMiscData->numHoles;
	assert(in_pBufferLengths[7] == sizeof(int) * in_pMiscData->numHoles);
	mesh->holes = (int*)os_malloc(sizeof(int) * in_pMiscData->numHoles); 
	memcpy(mesh->holes ,in_ppBufferPointers[7],in_pBufferLengths[7]);	
      }


    for (size_t i=0; i<numEdges; i++) mesh->subdivlevel[i] = 1.0f;
    int offset = 0;
    for (size_t i=0; i<numFaces; i++)
      {
        mesh->face_offsets[i] = offset;
        offset+=mesh->verticesPerFace[i];       
      }
 
    g_ispc_scene->subdiv[meshID] = mesh;
  }

  extern "C" void run_create_hairset(uint32_t         in_BufferCount,
				     void**           in_ppBufferPointers,
				     uint64_t*        in_pBufferLengths,
				     CreateHairSetData*  in_pMiscData,
				     uint16_t         in_MiscDataLength,
				     void*            in_pReturnValue,
				     uint16_t         in_ReturnValueLength)
  {
    size_t hairsetID = g_hairsetID++;
    ISPCHairSet* hairset = new ISPCHairSet(in_pMiscData->numHairs,in_pMiscData->numVertices);
    memcpy(hairset->positions = (Vec3fa*)malloc(in_pBufferLengths[0]),in_ppBufferPointers[0],in_pBufferLengths[0]);
    memcpy(hairset->hairs = (ISPCHair*)malloc(in_pBufferLengths[1]),in_ppBufferPointers[1],in_pBufferLengths[1]);
    g_ispc_scene->hairsets[hairsetID] = hairset;
  }

  extern "C" void run_create_scene(uint32_t         in_BufferCount,
                                   void**           in_ppBufferPointers,
                                   uint64_t*        in_pBufferLengths,
                                   CreateSceneData* in_pMiscData,
                                   uint16_t         in_MiscDataLength,
                                   void*            in_pReturnValue,
                                   uint16_t         in_ReturnValueLength)
  {
    g_meshID = 0;
    g_ispc_scene = new ISPCScene(in_pMiscData->numMeshes,
                                 in_pMiscData->numHairSets,
                                 in_ppBufferPointers[0],in_pMiscData->numMaterials,
                                 in_ppBufferPointers[1],in_pMiscData->numAmbientLights,
                                 in_ppBufferPointers[2],in_pMiscData->numPointLights,
                                 in_ppBufferPointers[3],in_pMiscData->numDirectionalLights,
                                 in_ppBufferPointers[4],in_pMiscData->numDistantLights,
				 in_pMiscData->numSubdivMeshes);
  }

  extern "C" void run_pick(uint32_t         in_BufferCount,
                           void**           in_ppBufferPointers,
                           uint64_t*        in_pBufferLengths,
                           PickDataSend*    in_pMiscData,
                           uint16_t         in_MiscDataLength,
                           PickDataReceive* in_pReturnValue,
                           uint16_t         in_ReturnValueLength)
  {
    Vec3fa hitPos = zero;
    bool hit = device_pick(in_pMiscData->x,
                           in_pMiscData->y,
                           in_pMiscData->vx,
                           in_pMiscData->vy,
                           in_pMiscData->vz,
                           in_pMiscData->p,
                           hitPos);
    in_pReturnValue->pos = hitPos;
    in_pReturnValue->hit = hit;
  }

  extern "C" void run_render(uint32_t         in_BufferCount,
                             void**           in_ppBufferPointers,
                             uint64_t*        in_pBufferLengths,
                             RenderData*      in_pMiscData,
                             uint16_t         in_MiscDataLength,
                             void*            in_pReturnValue,
                             uint16_t         in_ReturnValueLength)
  {
    //double t0 = getSeconds();
    device_render((int*)in_ppBufferPointers[0],
                  in_pMiscData->width,
                  in_pMiscData->height,
                  in_pMiscData->time, 
                  in_pMiscData->vx, 
                  in_pMiscData->vy, 
                  in_pMiscData->vz,
                  in_pMiscData->p);
    //double dt = getSeconds() - t0;
    //printf("render %3.2f fps, %.2f ms\n",1.0f/dt,dt*1000.0f); flush(std::cout);
  }

  extern "C" void run_cleanup(uint32_t         in_BufferCount,
                              void**           in_ppBufferPointers,
                              uint64_t*        in_pBufferLengths,
                              void*            in_pMiscData,
                              uint16_t         in_MiscDataLength,
                              void*            in_pReturnValue,
                              uint16_t         in_ReturnValueLength)
  {
    device_cleanup();
    if (g_ispc_scene) delete g_ispc_scene; g_ispc_scene = NULL;
  }
}

int main(int argc, char** argv) 
{
  UNUSED_ATTR COIRESULT result;
  UNREFERENCED_PARAM (argc);
  UNREFERENCED_PARAM (argv);

  /* enable wait to attach with debugger */
#if 0
  std::cout << "waiting for debugger to attach ..." << std::flush;
#if 0
  volatile int loop = 1;
  do {
    volatile int a = 1;
  } while (loop);
  
#else
  for (int i=0; i<20; i++) {
    sleep(1);
    std::cout << "." << std::flush;
  }
#endif
  std::cout << " [DONE]" << std::endl;
#endif
  
  // Functions enqueued on the sink side will not start executing until
  // you call COIPipelineStartExecutingRunFunctions(). This call is to
  // synchronize any initialization required on the sink side
  result = COIPipelineStartExecutingRunFunctions();
  assert(result == COI_SUCCESS);

  // This call will wait until COIProcessDestroy() gets called on the source
  // side. If COIProcessDestroy is called without force flag set, this call
  // will make sure all the functions enqueued are executed and does all
  // clean up required to exit gracefully.
  COIProcessWaitForShutdown();
  return 0;
}
