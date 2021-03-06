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

#include "bvh4.h"
#include "bvh4_statistics.h"

#include "geometry/bezier1v.h"
#include "geometry/bezier1i.h"
#include "geometry/triangle1.h"
#include "geometry/triangle4.h"
#include "geometry/triangle8.h"
#include "geometry/triangle1v.h"
#include "geometry/triangle4v.h"
#include "geometry/triangle4v_mb.h"
#include "geometry/triangle4i.h"
#include "geometry/subdivpatch1.h"
#include "geometry/subdivpatch1cached.h"
#include "geometry/virtual_accel.h"

#include "common/accelinstance.h"

namespace embree
{
  DECLARE_SYMBOL(Accel::Intersector1,BVH4Bezier1vIntersector1);
  DECLARE_SYMBOL(Accel::Intersector1,BVH4Bezier1iIntersector1);
  DECLARE_SYMBOL(Accel::Intersector1,BVH4Bezier1vIntersector1_OBB);
  DECLARE_SYMBOL(Accel::Intersector1,BVH4Bezier1iIntersector1_OBB);
  DECLARE_SYMBOL(Accel::Intersector1,BVH4Bezier1iMBIntersector1_OBB);
  DECLARE_SYMBOL(Accel::Intersector1,BVH4Triangle1Intersector1Moeller);
  DECLARE_SYMBOL(Accel::Intersector1,BVH4Triangle4Intersector1Moeller);
  DECLARE_SYMBOL(Accel::Intersector1,BVH4Triangle8Intersector1Moeller);
  DECLARE_SYMBOL(Accel::Intersector1,BVH4Triangle1vIntersector1Pluecker);
  DECLARE_SYMBOL(Accel::Intersector1,BVH4Triangle4vIntersector1Pluecker);
  DECLARE_SYMBOL(Accel::Intersector1,BVH4Triangle4iIntersector1Pluecker);
  DECLARE_SYMBOL(Accel::Intersector1,BVH4Triangle1vMBIntersector1Moeller);
  DECLARE_SYMBOL(Accel::Intersector1,BVH4Triangle4vMBIntersector1Moeller);
  DECLARE_SYMBOL(Accel::Intersector1,BVH4Subdivpatch1Intersector1);
  DECLARE_SYMBOL(Accel::Intersector1,BVH4Subdivpatch1CachedIntersector1);
  DECLARE_SYMBOL(Accel::Intersector1,BVH4GridIntersector1);
  DECLARE_SYMBOL(Accel::Intersector1,BVH4GridLazyIntersector1);
  DECLARE_SYMBOL(Accel::Intersector1,BVH4VirtualIntersector1);

  DECLARE_SYMBOL(Accel::Intersector4,BVH4Bezier1vIntersector4Chunk);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Bezier1iIntersector4Chunk);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Bezier1vIntersector4Single_OBB);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Bezier1iIntersector4Single_OBB);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Bezier1iMBIntersector4Single_OBB);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Triangle1Intersector4ChunkMoeller);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Triangle4Intersector4ChunkMoeller);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Triangle4Intersector4ChunkMoellerNoFilter);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Triangle8Intersector4ChunkMoeller);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Triangle8Intersector4ChunkMoellerNoFilter);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Triangle4Intersector4HybridMoeller);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Triangle4Intersector4HybridMoellerNoFilter);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Triangle8Intersector4HybridMoeller);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Triangle8Intersector4HybridMoellerNoFilter);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Triangle1vIntersector4ChunkPluecker);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Triangle4vIntersector4ChunkPluecker);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Triangle4vIntersector4HybridPluecker);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Triangle4iIntersector4ChunkPluecker);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Triangle1vMBIntersector4ChunkMoeller);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Triangle4vMBIntersector4ChunkMoeller);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Subdivpatch1Intersector4);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4Subdivpatch1CachedIntersector4);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4GridIntersector4);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4GridLazyIntersector4);
  DECLARE_SYMBOL(Accel::Intersector4,BVH4VirtualIntersector4Chunk);
  
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Bezier1vIntersector8Chunk);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Bezier1iIntersector8Chunk);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Bezier1vIntersector8Single_OBB);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Bezier1iIntersector8Single_OBB);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Bezier1iMBIntersector8Single_OBB);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Triangle1Intersector8ChunkMoeller);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Triangle4Intersector8ChunkMoeller);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Triangle4Intersector8ChunkMoellerNoFilter);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Triangle8Intersector8ChunkMoeller);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Triangle8Intersector8ChunkMoellerNoFilter);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Triangle4Intersector8HybridMoeller);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Triangle4Intersector8HybridMoellerNoFilter);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Triangle8Intersector8HybridMoeller);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Triangle8Intersector8HybridMoellerNoFilter);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Triangle1vIntersector8ChunkPluecker);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Triangle4vIntersector8ChunkPluecker);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Triangle4vIntersector8HybridPluecker);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Triangle4iIntersector8ChunkPluecker);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Triangle1vMBIntersector8ChunkMoeller);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Triangle4vMBIntersector8ChunkMoeller);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Subdivpatch1Intersector8);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4Subdivpatch1CachedIntersector8);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4GridIntersector8);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4GridLazyIntersector8);
  DECLARE_SYMBOL(Accel::Intersector8,BVH4VirtualIntersector8Chunk);

  DECLARE_TOPLEVEL_BUILDER(BVH4BuilderTwoLevelSAH);

  DECLARE_SCENE_BUILDER(BVH4Bezier1vBuilder_OBB_New);
  DECLARE_SCENE_BUILDER(BVH4Bezier1iBuilder_OBB_New);
  DECLARE_SCENE_BUILDER(BVH4Bezier1iMBBuilder_OBB_New);

  DECLARE_SCENE_BUILDER(BVH4Triangle1SceneBuilderSAH);
  DECLARE_SCENE_BUILDER(BVH4Triangle4SceneBuilderSAH);
  DECLARE_SCENE_BUILDER(BVH4Triangle8SceneBuilderSAH);
  DECLARE_SCENE_BUILDER(BVH4Triangle1vSceneBuilderSAH);
  DECLARE_SCENE_BUILDER(BVH4Triangle4vSceneBuilderSAH);
  DECLARE_SCENE_BUILDER(BVH4Triangle4iSceneBuilderSAH);
  DECLARE_SCENE_BUILDER(BVH4Triangle4vMBSceneBuilderSAH);

  DECLARE_SCENE_BUILDER(BVH4Triangle1SceneBuilderSpatialSAH);
  DECLARE_SCENE_BUILDER(BVH4Triangle4SceneBuilderSpatialSAH);
  DECLARE_SCENE_BUILDER(BVH4Triangle8SceneBuilderSpatialSAH);
  DECLARE_SCENE_BUILDER(BVH4Triangle1vSceneBuilderSpatialSAH);
  DECLARE_SCENE_BUILDER(BVH4Triangle4vSceneBuilderSpatialSAH);
  DECLARE_SCENE_BUILDER(BVH4Triangle4iSceneBuilderSpatialSAH);

  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle1MeshBuilderSAH);
  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle4MeshBuilderSAH);
  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle8MeshBuilderSAH);
  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle1vMeshBuilderSAH);
  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle4vMeshBuilderSAH);
  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle4iMeshBuilderSAH);
  DECLARE_SCENE_BUILDER(BVH4Bezier1vSceneBuilderSAH);
  DECLARE_SCENE_BUILDER(BVH4Bezier1iSceneBuilderSAH);
  DECLARE_SCENE_BUILDER(BVH4VirtualSceneBuilderSAH);

  DECLARE_SCENE_BUILDER(BVH4SubdivPatch1BuilderBinnedSAH);
  DECLARE_SCENE_BUILDER(BVH4SubdivPatch1CachedBuilderBinnedSAH);
  DECLARE_SCENE_BUILDER(BVH4SubdivGridBuilderBinnedSAH);
  DECLARE_SCENE_BUILDER(BVH4SubdivGridEagerBuilderBinnedSAH);
  DECLARE_SCENE_BUILDER(BVH4SubdivGridLazyBuilderBinnedSAH);

  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle1MeshRefitSAH);
  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle4MeshRefitSAH);
  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle8MeshRefitSAH);
  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle1vMeshRefitSAH);
  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle4vMeshRefitSAH);
  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle4iMeshRefitSAH);

  DECLARE_SCENE_BUILDER(BVH4Triangle1SceneBuilderMortonGeneral);
  DECLARE_SCENE_BUILDER(BVH4Triangle4SceneBuilderMortonGeneral);
  DECLARE_SCENE_BUILDER(BVH4Triangle8SceneBuilderMortonGeneral);
  DECLARE_SCENE_BUILDER(BVH4Triangle1vSceneBuilderMortonGeneral);
  DECLARE_SCENE_BUILDER(BVH4Triangle4vSceneBuilderMortonGeneral);
  DECLARE_SCENE_BUILDER(BVH4Triangle4iSceneBuilderMortonGeneral);

  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle1MeshBuilderMortonGeneral);
  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle4MeshBuilderMortonGeneral);
  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle8MeshBuilderMortonGeneral);
  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle1vMeshBuilderMortonGeneral);
  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle4vMeshBuilderMortonGeneral);
  DECLARE_TRIANGLEMESH_BUILDER(BVH4Triangle4iMeshBuilderMortonGeneral);

  void BVH4Register () 
  {
    int features = getCPUFeatures();

    /* select builders */
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4BuilderTwoLevelSAH);

    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Bezier1vBuilder_OBB_New);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Bezier1iBuilder_OBB_New);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Bezier1iMBBuilder_OBB_New);

    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle1SceneBuilderSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4SceneBuilderSAH);
    SELECT_SYMBOL_AVX        (features,BVH4Triangle8SceneBuilderSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle1vSceneBuilderSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4vSceneBuilderSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4iSceneBuilderSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4vMBSceneBuilderSAH);

    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle1SceneBuilderSpatialSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4SceneBuilderSpatialSAH);
    SELECT_SYMBOL_AVX        (features,BVH4Triangle8SceneBuilderSpatialSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle1vSceneBuilderSpatialSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4vSceneBuilderSpatialSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4iSceneBuilderSpatialSAH);
    
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle1MeshBuilderSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4MeshBuilderSAH);
    SELECT_SYMBOL_AVX        (features,BVH4Triangle8MeshBuilderSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle1vMeshBuilderSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4vMeshBuilderSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4iMeshBuilderSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Bezier1vSceneBuilderSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Bezier1iSceneBuilderSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4VirtualSceneBuilderSAH);

    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4SubdivPatch1BuilderBinnedSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4SubdivPatch1CachedBuilderBinnedSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4SubdivGridBuilderBinnedSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4SubdivGridEagerBuilderBinnedSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4SubdivGridLazyBuilderBinnedSAH);

    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle1MeshRefitSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4MeshRefitSAH);
    SELECT_SYMBOL_AVX        (features,BVH4Triangle8MeshRefitSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle1vMeshRefitSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4vMeshRefitSAH);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4iMeshRefitSAH);

    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle1SceneBuilderMortonGeneral);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4SceneBuilderMortonGeneral);
    SELECT_SYMBOL_AVX        (features,BVH4Triangle8SceneBuilderMortonGeneral);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle1vSceneBuilderMortonGeneral);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4vSceneBuilderMortonGeneral);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4iSceneBuilderMortonGeneral);

    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle1MeshBuilderMortonGeneral);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4MeshBuilderMortonGeneral);
    SELECT_SYMBOL_AVX        (features,BVH4Triangle8MeshBuilderMortonGeneral);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle1vMeshBuilderMortonGeneral);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4vMeshBuilderMortonGeneral);
    SELECT_SYMBOL_DEFAULT_AVX(features,BVH4Triangle4iMeshBuilderMortonGeneral);

    /* select intersectors1 */
    SELECT_SYMBOL_DEFAULT_AVX_AVX2      (features,BVH4Bezier1vIntersector1);
    SELECT_SYMBOL_DEFAULT_AVX_AVX2      (features,BVH4Bezier1iIntersector1);
    SELECT_SYMBOL_DEFAULT_AVX_AVX2      (features,BVH4Bezier1vIntersector1_OBB);
    SELECT_SYMBOL_DEFAULT_AVX_AVX2      (features,BVH4Bezier1iIntersector1_OBB);
    SELECT_SYMBOL_DEFAULT_AVX_AVX2      (features,BVH4Bezier1iMBIntersector1_OBB);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX_AVX2(features,BVH4Triangle1Intersector1Moeller);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX_AVX2(features,BVH4Triangle4Intersector1Moeller);
    SELECT_SYMBOL_AVX_AVX2              (features,BVH4Triangle8Intersector1Moeller);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX     (features,BVH4Triangle1vIntersector1Pluecker);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX     (features,BVH4Triangle4vIntersector1Pluecker);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX     (features,BVH4Triangle4iIntersector1Pluecker);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX_AVX2(features,BVH4Triangle1vMBIntersector1Moeller);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX_AVX2(features,BVH4Triangle4vMBIntersector1Moeller);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX_AVX2(features,BVH4Subdivpatch1Intersector1);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX_AVX2(features,BVH4Subdivpatch1CachedIntersector1);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX_AVX2(features,BVH4GridIntersector1);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX_AVX2(features,BVH4GridLazyIntersector1);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX_AVX2(features,BVH4VirtualIntersector1);

    /* select intersectors4 */
    SELECT_SYMBOL_DEFAULT_AVX_AVX2      (features,BVH4Bezier1vIntersector4Chunk);
    SELECT_SYMBOL_DEFAULT_AVX_AVX2      (features,BVH4Bezier1iIntersector4Chunk);
    SELECT_SYMBOL_DEFAULT_AVX_AVX2      (features,BVH4Bezier1vIntersector4Single_OBB);
    SELECT_SYMBOL_DEFAULT_AVX_AVX2      (features,BVH4Bezier1iIntersector4Single_OBB);
    SELECT_SYMBOL_DEFAULT_AVX_AVX2      (features,BVH4Bezier1iMBIntersector4Single_OBB);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX_AVX2(features,BVH4Triangle1Intersector4ChunkMoeller);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX_AVX2(features,BVH4Triangle4Intersector4ChunkMoeller);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX_AVX2(features,BVH4Triangle4Intersector4ChunkMoellerNoFilter);
    SELECT_SYMBOL_AVX_AVX2              (features,BVH4Triangle8Intersector4ChunkMoeller);
    SELECT_SYMBOL_DEFAULT2              (features,BVH4Triangle4Intersector4HybridMoeller,BVH4Triangle4Intersector4ChunkMoeller); // hybrid not supported below SSE4.2
    SELECT_SYMBOL_AVX_AVX2              (features,BVH4Triangle8Intersector4ChunkMoellerNoFilter);
    SELECT_SYMBOL_DEFAULT2              (features,BVH4Triangle4Intersector4HybridMoellerNoFilter,BVH4Triangle4Intersector4ChunkMoellerNoFilter); // hybrid not supported below SSE4.2
    SELECT_SYMBOL_SSE42_AVX_AVX2        (features,BVH4Triangle4Intersector4HybridMoeller);
    SELECT_SYMBOL_SSE42_AVX_AVX2        (features,BVH4Triangle4Intersector4HybridMoellerNoFilter);
    SELECT_SYMBOL_AVX_AVX2              (features,BVH4Triangle8Intersector4HybridMoeller);
    SELECT_SYMBOL_AVX_AVX2              (features,BVH4Triangle8Intersector4HybridMoellerNoFilter);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX     (features,BVH4Triangle1vIntersector4ChunkPluecker);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX     (features,BVH4Triangle4vIntersector4ChunkPluecker);
    SELECT_SYMBOL_DEFAULT2              (features,BVH4Triangle4vIntersector4HybridPluecker,BVH4Triangle4vIntersector4ChunkPluecker); // hybrid not supported below SSE4.2
    SELECT_SYMBOL_SSE42_AVX             (features,BVH4Triangle4vIntersector4HybridPluecker);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX     (features,BVH4Triangle4iIntersector4ChunkPluecker);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX_AVX2(features,BVH4Triangle1vMBIntersector4ChunkMoeller);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX_AVX2(features,BVH4Triangle4vMBIntersector4ChunkMoeller);
    SELECT_SYMBOL_DEFAULT_AVX_AVX2      (features,BVH4Subdivpatch1Intersector4);
    SELECT_SYMBOL_DEFAULT_AVX_AVX2      (features,BVH4Subdivpatch1CachedIntersector4);
    SELECT_SYMBOL_DEFAULT_AVX_AVX2      (features,BVH4GridIntersector4);
    SELECT_SYMBOL_DEFAULT_AVX_AVX2      (features,BVH4GridLazyIntersector4);
    SELECT_SYMBOL_DEFAULT_SSE41_AVX_AVX2(features,BVH4VirtualIntersector4Chunk);
   
    /* select intersectors8 */
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Bezier1vIntersector8Chunk);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Bezier1iIntersector8Chunk);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Bezier1vIntersector8Single_OBB);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Bezier1iIntersector8Single_OBB);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Bezier1iMBIntersector8Single_OBB);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Triangle1Intersector8ChunkMoeller);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Triangle4Intersector8ChunkMoeller);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Triangle4Intersector8ChunkMoellerNoFilter);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Triangle8Intersector8ChunkMoeller);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Triangle8Intersector8ChunkMoellerNoFilter);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Triangle4Intersector8HybridMoeller);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Triangle4Intersector8HybridMoellerNoFilter);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Triangle8Intersector8HybridMoeller);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Triangle8Intersector8HybridMoellerNoFilter);
    SELECT_SYMBOL_AVX     (features,BVH4Triangle1vIntersector8ChunkPluecker);
    SELECT_SYMBOL_AVX     (features,BVH4Triangle4vIntersector8ChunkPluecker);
    SELECT_SYMBOL_AVX     (features,BVH4Triangle4vIntersector8HybridPluecker);
    SELECT_SYMBOL_AVX     (features,BVH4Triangle4iIntersector8ChunkPluecker);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Triangle1vMBIntersector8ChunkMoeller);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Triangle4vMBIntersector8ChunkMoeller);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Subdivpatch1Intersector8);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4Subdivpatch1CachedIntersector8);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4GridIntersector8);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4GridLazyIntersector8);
    SELECT_SYMBOL_AVX_AVX2(features,BVH4VirtualIntersector8Chunk);
  }

  BVH4::BVH4 (const PrimitiveType& primTy, Scene* scene, bool listMode)
    : primTy(primTy), scene(scene), listMode(listMode),
      root(emptyNode), numPrimitives(0), numVertices(0), data_mem(NULL), size_data_mem(0) {}

  BVH4::~BVH4 () 
  {
    for (size_t i=0; i<objects.size(); i++) 
      delete objects[i];
    
    if (data_mem) {
      os_free( data_mem, size_data_mem );        
      data_mem = NULL;
      size_data_mem = 0;
    }
  }

  void BVH4::clear() 
  {
    set(BVH4::emptyNode,empty,0);
    alloc.clear();
  }

  void BVH4::set (NodeRef root, const BBox3fa& bounds, size_t numPrimitives)
  {
    this->root = root;
    this->bounds = bounds;
    this->numPrimitives = numPrimitives;
  }

  void BVH4::printStatistics()
  {
    std::cout << BVH4Statistics(this).str();
    std::cout << "  "; alloc.print_statistics();
  }	

  void BVH4::clearBarrier(NodeRef& node)
  {
    if (node.isBarrier())
      node.clearBarrier();
    else if (!node.isLeaf()) {
      Node* n = node.node();
      for (size_t c=0; c<N; c++)
        clearBarrier(n->child(c));
    }
  }

  void BVH4::layoutLargeNodes(size_t N)
  {
    struct NodeArea 
    {
      __forceinline NodeArea() {}

      __forceinline NodeArea(NodeRef& node, const BBox3fa& bounds)
        : node(&node), A(node.isLeaf() ? float(neg_inf) : area(bounds)) {}

      __forceinline bool operator< (const NodeArea& other) const {
        return this->A < other.A;
      }

      NodeRef* node;
      float A;
    };
    std::vector<NodeArea> lst;
    lst.reserve(N);
    lst.push_back(NodeArea(root,empty));

    while (lst.size() < N)
    {
      std::pop_heap(lst.begin(), lst.end());
      NodeArea n = lst.back(); lst.pop_back();
      if (!n.node->isNode()) break;
      Node* node = n.node->node();
      for (size_t i=0; i<BVH4::N; i++) {
        if (node->child(i) == BVH4::emptyNode) continue;
        lst.push_back(NodeArea(node->child(i),node->bounds(i)));
        std::push_heap(lst.begin(), lst.end());
      }
    }

    for (size_t i=0; i<lst.size(); i++)
      lst[i].node->setBarrier();
      
    root = layoutLargeNodesRecursion(root);
  }
  
  BVH4::NodeRef BVH4::layoutLargeNodesRecursion(NodeRef& node)
  {
    if (node.isBarrier()) {
      node.clearBarrier();
      return node;
    }
    else if (node.isNode()) 
    {
      Node* oldnode = node.node();
      Node* newnode = (BVH4::Node*) alloc.threadLocal2()->alloc0.malloc(sizeof(BVH4::Node)); // FIXME: optimize access to threadLocal2 
      *newnode = *oldnode;
      for (size_t c=0; c<BVH4::N; c++)
        newnode->child(c) = layoutLargeNodesRecursion(oldnode->child(c));
      return encodeNode(newnode);
    }
    else return node;
  }

  std::pair<BBox3fa,BBox3fa> BVH4::refit(Scene* scene, NodeRef node)
  {
    /*! merge bounds of triangles for both time steps */
    if (node.isLeaf()) 
    {
      size_t num; char* tri = node.leaf(num);
      if (node == BVH4::emptyNode) return std::pair<BBox3fa,BBox3fa>(empty,empty);
      return primTy.update2(tri,listMode ? -1 : num,scene);
    }
    /*! set and propagate merged bounds for both time steps */
    else
    {
      NodeMB* n = node.nodeMB();
      if (!n->hasBounds()) {
        for (size_t i=0; i<4; i++) {
          std::pair<BBox3fa,BBox3fa> bounds = refit(scene,n->child(i));
          n->set(i,bounds.first,bounds.second);
        }
      }
      BBox3fa bounds0 = merge(n->bounds0(0),n->bounds0(1),n->bounds0(2),n->bounds0(3));
      BBox3fa bounds1 = merge(n->bounds1(0),n->bounds1(1),n->bounds1(2),n->bounds1(3));
      return std::pair<BBox3fa,BBox3fa>(bounds0,bounds1);
    }
  }

  double BVH4::preBuild(const char* builderName)
  {
    if (builderName == NULL) 
      return inf;

    if (g_verbose >= 1)
      std::cout << "building BVH4<" << primTy.name << "> using " << builderName << " ..." << std::flush;

    double t0 = 0.0;
    if (g_benchmark || g_verbose >= 1) t0 = getSeconds();
    return t0;
  }  

  void BVH4::postBuild(double t0)
  {
    //if (staticGeom) alloc.shrink(); // FIXME: triggers bug

    if (t0 == double(inf))
      return;
    
    double dt = 0.0;
    if (g_benchmark || g_verbose >= 1) 
      dt = getSeconds()-t0;

    /* print statistics */
    if (g_verbose >= 1) {
      std::cout << " [DONE]" << "  " << 1000.0f*dt << "ms (" << 1E-6*double(numPrimitives)/dt << " Mprim/s)" << std::endl;
    }
    
    if (g_verbose >= 2)
      printStatistics();
    
    /* benchmark mode */
    if (g_benchmark) {
      BVH4Statistics stat(this);
      std::cout << "BENCHMARK_BUILD " << dt << " " << double(numPrimitives)/dt << " " << stat.sah() << " " << stat.bytesUsed() << std::endl;
    }
  }

  Accel::Intersectors BVH4Bezier1vIntersectors(BVH4* bvh)
  {
    Accel::Intersectors intersectors;
    intersectors.ptr = bvh;
    intersectors.intersector1 = BVH4Bezier1vIntersector1;
    intersectors.intersector4 = BVH4Bezier1vIntersector4Chunk;
    intersectors.intersector8 = BVH4Bezier1vIntersector8Chunk;
    intersectors.intersector16 = NULL;
    return intersectors;
  }
  
  Accel::Intersectors BVH4Bezier1iIntersectors(BVH4* bvh)
  {
    Accel::Intersectors intersectors;
    intersectors.ptr = bvh;
    intersectors.intersector1 = BVH4Bezier1iIntersector1;
    intersectors.intersector4 = BVH4Bezier1iIntersector4Chunk;
    intersectors.intersector8 = BVH4Bezier1iIntersector8Chunk;
    intersectors.intersector16 = NULL;
    return intersectors;
  }

  Accel::Intersectors BVH4Bezier1vIntersectors_OBB(BVH4* bvh)
  {
    Accel::Intersectors intersectors;
    intersectors.ptr = bvh;
    intersectors.intersector1 = BVH4Bezier1vIntersector1_OBB;
    intersectors.intersector4 = BVH4Bezier1vIntersector4Single_OBB;
    intersectors.intersector8 = BVH4Bezier1vIntersector8Single_OBB;
    intersectors.intersector16 = NULL;
    return intersectors;
  }
  
  Accel::Intersectors BVH4Bezier1iIntersectors_OBB(BVH4* bvh)
  {
    Accel::Intersectors intersectors;
    intersectors.ptr = bvh;
    intersectors.intersector1 = BVH4Bezier1iIntersector1_OBB;
    intersectors.intersector4 = BVH4Bezier1iIntersector4Single_OBB;
    intersectors.intersector8 = BVH4Bezier1iIntersector8Single_OBB;
    intersectors.intersector16 = NULL;
    return intersectors;
  }

  Accel::Intersectors BVH4Bezier1iMBIntersectors_OBB(BVH4* bvh)
  {
    Accel::Intersectors intersectors;
    intersectors.ptr = bvh;
    intersectors.intersector1 = BVH4Bezier1iMBIntersector1_OBB;
    intersectors.intersector4 = BVH4Bezier1iMBIntersector4Single_OBB;
    intersectors.intersector8 = BVH4Bezier1iMBIntersector8Single_OBB;
    intersectors.intersector16 = NULL;
    return intersectors;
  }
  
  Accel::Intersectors BVH4Triangle1Intersectors(BVH4* bvh)
  {
    Accel::Intersectors intersectors;
    intersectors.ptr = bvh;
    intersectors.intersector1 = BVH4Triangle1Intersector1Moeller;
    intersectors.intersector4 = BVH4Triangle1Intersector4ChunkMoeller;
    intersectors.intersector8 = BVH4Triangle1Intersector8ChunkMoeller;
    intersectors.intersector16 = NULL;
    return intersectors;
  }

  Accel::Intersectors BVH4Triangle4IntersectorsChunk(BVH4* bvh)
  {
    Accel::Intersectors intersectors;
    intersectors.ptr = bvh;
    intersectors.intersector1 = BVH4Triangle4Intersector1Moeller;
    intersectors.intersector4_filter   = BVH4Triangle4Intersector4ChunkMoeller;
    intersectors.intersector4_nofilter = BVH4Triangle4Intersector4ChunkMoellerNoFilter;
    intersectors.intersector8_filter   = BVH4Triangle4Intersector8ChunkMoeller;
    intersectors.intersector8_nofilter = BVH4Triangle4Intersector8ChunkMoellerNoFilter;
    intersectors.intersector16 = NULL;
    return intersectors;
  }

  Accel::Intersectors BVH4Triangle4IntersectorsHybrid(BVH4* bvh)
  {
    Accel::Intersectors intersectors;
    intersectors.ptr = bvh;
    intersectors.intersector1 = BVH4Triangle4Intersector1Moeller;
    intersectors.intersector4_filter   = BVH4Triangle4Intersector4HybridMoeller;
    intersectors.intersector4_nofilter = BVH4Triangle4Intersector4HybridMoellerNoFilter;
    intersectors.intersector8_filter = BVH4Triangle4Intersector8HybridMoeller;
    intersectors.intersector8_nofilter = BVH4Triangle4Intersector8HybridMoellerNoFilter;
    intersectors.intersector16 = NULL;
    return intersectors;
  }

  Accel::Intersectors BVH4Triangle8IntersectorsChunk(BVH4* bvh)
  {
    Accel::Intersectors intersectors;
    intersectors.ptr = bvh;
    intersectors.intersector1 = BVH4Triangle8Intersector1Moeller;
    intersectors.intersector4_filter   = BVH4Triangle8Intersector4ChunkMoeller;
    intersectors.intersector4_nofilter = BVH4Triangle8Intersector4ChunkMoellerNoFilter;
    intersectors.intersector8 = BVH4Triangle8Intersector8ChunkMoeller;
    intersectors.intersector8_filter   = BVH4Triangle8Intersector8ChunkMoeller;
    intersectors.intersector8_nofilter = BVH4Triangle8Intersector8ChunkMoellerNoFilter;
    intersectors.intersector16 = NULL;
    return intersectors;
  }

  Accel::Intersectors BVH4Triangle8IntersectorsHybrid(BVH4* bvh)
  {
    Accel::Intersectors intersectors;
    intersectors.ptr = bvh;
    intersectors.intersector1 = BVH4Triangle8Intersector1Moeller;
    intersectors.intersector4_filter   = BVH4Triangle8Intersector4HybridMoeller;
    intersectors.intersector4_nofilter = BVH4Triangle8Intersector4HybridMoellerNoFilter;
    intersectors.intersector8_filter   = BVH4Triangle8Intersector8HybridMoeller;
    intersectors.intersector8_nofilter = BVH4Triangle8Intersector8HybridMoellerNoFilter;
    intersectors.intersector16 = NULL;
    return intersectors;
  }

  Accel::Intersectors BVH4Triangle1vIntersectors(BVH4* bvh)
  {
    Accel::Intersectors intersectors;
    intersectors.ptr = bvh;
    intersectors.intersector1 = BVH4Triangle1vIntersector1Pluecker;
    intersectors.intersector4 = BVH4Triangle1vIntersector4ChunkPluecker;
    intersectors.intersector8 = BVH4Triangle1vIntersector8ChunkPluecker;
    intersectors.intersector16 = NULL;
    return intersectors;
  }

  Accel::Intersectors BVH4Triangle1vMBIntersectors(BVH4* bvh)
  {
    Accel::Intersectors intersectors;
    intersectors.ptr = bvh;
    intersectors.intersector1 = BVH4Triangle1vMBIntersector1Moeller;
    intersectors.intersector4 = BVH4Triangle1vMBIntersector4ChunkMoeller;
    intersectors.intersector8 = BVH4Triangle1vMBIntersector8ChunkMoeller;
    intersectors.intersector16 = NULL;
    return intersectors;
  }

  Accel::Intersectors BVH4Triangle4vMBIntersectors(BVH4* bvh)
  {
    Accel::Intersectors intersectors;
    intersectors.ptr = bvh;
    intersectors.intersector1 = BVH4Triangle4vMBIntersector1Moeller;
    intersectors.intersector4 = BVH4Triangle4vMBIntersector4ChunkMoeller;
    intersectors.intersector8 = BVH4Triangle4vMBIntersector8ChunkMoeller;
    intersectors.intersector16 = NULL;
    return intersectors;
  }

  Accel::Intersectors BVH4Triangle4vIntersectorsChunk(BVH4* bvh)
  {
    Accel::Intersectors intersectors;
    intersectors.ptr = bvh;
    intersectors.intersector1 = BVH4Triangle4vIntersector1Pluecker;
    intersectors.intersector4 = BVH4Triangle4vIntersector4ChunkPluecker;
    intersectors.intersector8 = BVH4Triangle4vIntersector8HybridPluecker;
    intersectors.intersector16 = NULL;
    return intersectors;
  }

  Accel::Intersectors BVH4Triangle4vIntersectorsHybrid(BVH4* bvh)
  {
    Accel::Intersectors intersectors;
    intersectors.ptr = bvh;
    intersectors.intersector1 = BVH4Triangle4vIntersector1Pluecker;
    intersectors.intersector4 = BVH4Triangle4vIntersector4HybridPluecker;
    intersectors.intersector8 = BVH4Triangle4vIntersector8HybridPluecker;
    intersectors.intersector16 = NULL;
    return intersectors;
  }

  Accel::Intersectors BVH4Triangle4iIntersectors(BVH4* bvh)
  {
    Accel::Intersectors intersectors;
    intersectors.ptr = bvh;
    intersectors.intersector1 = BVH4Triangle4iIntersector1Pluecker;
    intersectors.intersector4 = BVH4Triangle4iIntersector4ChunkPluecker;
    intersectors.intersector8 = BVH4Triangle4iIntersector8ChunkPluecker;
    intersectors.intersector16 = NULL;
    return intersectors;
  }

  Accel* BVH4::BVH4Bezier1v(Scene* scene)
  { 
    BVH4* accel = new BVH4(Bezier1vType::type,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Bezier1vIntersectors(accel);
    Builder* builder = BVH4Bezier1vSceneBuilderSAH(accel,scene,LeafMode);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4Bezier1i(Scene* scene)
  { 
    BVH4* accel = new BVH4(SceneBezier1i::type,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Bezier1iIntersectors(accel);
    Builder* builder = BVH4Bezier1iSceneBuilderSAH(accel,scene,LeafMode);
    scene->needVertices = true;
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4OBBBezier1v(Scene* scene, bool highQuality)
  { 
    BVH4* accel = new BVH4(Bezier1vType::type,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Bezier1vIntersectors_OBB(accel);
    Builder* builder = BVH4Bezier1vBuilder_OBB_New(accel,scene,MODE_HIGH_QUALITY); // FIXME: enable high quality mode
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4OBBBezier1i(Scene* scene, bool highQuality)
  { 
    BVH4* accel = new BVH4(SceneBezier1i::type,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Bezier1iIntersectors_OBB(accel);
    Builder* builder = BVH4Bezier1iBuilder_OBB_New(accel,scene,MODE_HIGH_QUALITY); // FIXME: enable high quality mode
    scene->needVertices = true;
    return new AccelInstance(accel,builder,intersectors);
  }

   Accel* BVH4::BVH4OBBBezier1iMB(Scene* scene, bool highQuality)
  { 
    scene->needVertices = true;
    BVH4* accel = new BVH4(Bezier1iMBType::type,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Bezier1iMBIntersectors_OBB(accel);
    Builder* builder = BVH4Bezier1iMBBuilder_OBB_New(accel,scene,MODE_HIGH_QUALITY); // FIXME: support high quality mode
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4Triangle1(Scene* scene)
  { 
    BVH4* accel = new BVH4(Triangle1Type::type,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle1Intersectors(accel);
    
    Builder* builder = NULL;
    if      (g_tri_builder == "default"     ) builder = BVH4Triangle1SceneBuilderSAH(accel,scene,0);
    else if (g_tri_builder == "sah"         ) builder = BVH4Triangle1SceneBuilderSAH(accel,scene,0);
    else if (g_tri_builder == "sah_presplit") builder = BVH4Triangle1SceneBuilderSAH(accel,scene,MODE_HIGH_QUALITY);
    else if (g_tri_builder == "morton"      ) builder = BVH4Triangle1SceneBuilderMortonGeneral(accel,scene,0);
    else THROW_RUNTIME_ERROR("unknown builder "+g_tri_builder+" for BVH4<Triangle1>");

    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4Triangle4(Scene* scene)
  { 
    BVH4* accel = new BVH4(Triangle4Type::type,scene,LeafMode);

    Accel::Intersectors intersectors;
    if      (g_tri_traverser == "default") intersectors = BVH4Triangle4IntersectorsHybrid(accel);
    else if (g_tri_traverser == "chunk"  ) intersectors = BVH4Triangle4IntersectorsChunk(accel);
    else if (g_tri_traverser == "hybrid" ) intersectors = BVH4Triangle4IntersectorsHybrid(accel);
    else THROW_RUNTIME_ERROR("unknown traverser "+g_tri_traverser+" for BVH4<Triangle4>");
   
    Builder* builder = NULL;
    if      (g_tri_builder == "default"     ) builder = BVH4Triangle4SceneBuilderSAH(accel,scene,0);
    else if (g_tri_builder == "sah"         ) builder = BVH4Triangle4SceneBuilderSAH(accel,scene,0);
    else if (g_tri_builder == "sah_spatial" ) builder = BVH4Triangle4SceneBuilderSpatialSAH(accel,scene,0);
    else if (g_tri_builder == "sah_presplit") builder = BVH4Triangle4SceneBuilderSAH(accel,scene,MODE_HIGH_QUALITY);
    else if (g_tri_builder == "morton"      ) builder = BVH4Triangle4SceneBuilderMortonGeneral(accel,scene,0);
    else THROW_RUNTIME_ERROR("unknown builder "+g_tri_builder+" for BVH4<Triangle4>");

    return new AccelInstance(accel,builder,intersectors);
  }

#if defined (__TARGET_AVX__)
  Accel* BVH4::BVH4Triangle8(Scene* scene)
  { 
    BVH4* accel = new BVH4(Triangle8Type::type,scene,LeafMode);

    Accel::Intersectors intersectors;
    if      (g_tri_traverser == "default") intersectors = BVH4Triangle8IntersectorsHybrid(accel);
    else if (g_tri_traverser == "chunk"  ) intersectors = BVH4Triangle8IntersectorsChunk(accel);
    else if (g_tri_traverser == "hybrid" ) intersectors = BVH4Triangle8IntersectorsHybrid(accel);
    else THROW_RUNTIME_ERROR("unknown traverser "+g_tri_traverser+" for BVH4<Triangle8>");
   
    Builder* builder = NULL;
    if      (g_tri_builder == "default"     ) builder = BVH4Triangle8SceneBuilderSAH(accel,scene,0);
    else if (g_tri_builder == "sah"         ) builder = BVH4Triangle8SceneBuilderSAH(accel,scene,0);
    else if (g_tri_builder == "sah_spatial" ) builder = BVH4Triangle8SceneBuilderSpatialSAH(accel,scene,0);
    else if (g_tri_builder == "sah_presplit") builder = BVH4Triangle8SceneBuilderSAH(accel,scene,MODE_HIGH_QUALITY);
    else if (g_tri_builder == "morton"      ) builder = BVH4Triangle8SceneBuilderMortonGeneral(accel,scene,0);
    else THROW_RUNTIME_ERROR("unknown builder "+g_tri_builder+" for BVH4<Triangle8>");

    return new AccelInstance(accel,builder,intersectors);
  }
#endif

  Accel* BVH4::BVH4Triangle1v(Scene* scene)
  {
    BVH4* accel = new BVH4(Triangle1vType::type,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle1vIntersectors(accel);

    Builder* builder = NULL;
    if      (g_tri_builder == "default"     ) builder = BVH4Triangle1vSceneBuilderSAH(accel,scene,0);
    else if (g_tri_builder == "sah"         ) builder = BVH4Triangle1vSceneBuilderSAH(accel,scene,0);
    else if (g_tri_builder == "sah_spatial" ) builder = BVH4Triangle1vSceneBuilderSpatialSAH(accel,scene,0);
    else if (g_tri_builder == "sah_presplit") builder = BVH4Triangle1vSceneBuilderSAH(accel,scene,MODE_HIGH_QUALITY);
    else if (g_tri_builder == "morton"      ) builder = BVH4Triangle1vSceneBuilderMortonGeneral(accel,scene,0);
    else THROW_RUNTIME_ERROR("unknown builder "+g_tri_builder+" for BVH4<Triangle1v>");
        
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4Triangle4vMB(Scene* scene)
  {
    BVH4* accel = new BVH4(Triangle4vMB::type,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle4vMBIntersectors(accel);
    Builder* builder = NULL;
    if       (g_tri_builder_mb == "default"    ) builder = BVH4Triangle4vMBSceneBuilderSAH(accel,scene,0);
    else  if (g_tri_builder_mb == "sah") builder = BVH4Triangle4vMBSceneBuilderSAH(accel,scene,0);
    else THROW_RUNTIME_ERROR("unknown builder "+g_tri_builder_mb+" for BVH4<Triangle4vMB>");
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4Triangle4v(Scene* scene)
  {
    BVH4* accel = new BVH4(Triangle4vType::type,scene,LeafMode);

    Accel::Intersectors intersectors;
    if      (g_tri_traverser == "default") intersectors = BVH4Triangle4vIntersectorsHybrid(accel);
    else if (g_tri_traverser == "chunk"  ) intersectors = BVH4Triangle4vIntersectorsChunk(accel);
    else if (g_tri_traverser == "hybrid" ) intersectors = BVH4Triangle4vIntersectorsHybrid(accel);
    else THROW_RUNTIME_ERROR("unknown traverser "+g_tri_traverser+" for BVH4<Triangle4>");

    Builder* builder = NULL;
    if      (g_tri_builder == "default"     ) builder = BVH4Triangle4vSceneBuilderSAH(accel,scene,0);
    else if (g_tri_builder == "sah"         ) builder = BVH4Triangle4vSceneBuilderSAH(accel,scene,0);
    else if (g_tri_builder == "sah_spatial" ) builder = BVH4Triangle4vSceneBuilderSpatialSAH(accel,scene,0);
    else if (g_tri_builder == "sah_presplit") builder = BVH4Triangle4vSceneBuilderSAH(accel,scene,MODE_HIGH_QUALITY);
    else if (g_tri_builder == "morton"      ) builder = BVH4Triangle4vSceneBuilderMortonGeneral(accel,scene,0);
    else THROW_RUNTIME_ERROR("unknown builder "+g_tri_builder+" for BVH4<Triangle4v>");

    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4Triangle4i(Scene* scene)
  {
    BVH4* accel = new BVH4(Triangle4iType::type,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle4iIntersectors(accel);

    Builder* builder = NULL;
    if      (g_tri_builder == "default"     ) builder = BVH4Triangle4iSceneBuilderSAH(accel,scene,0);
    else if (g_tri_builder == "sah"         ) builder = BVH4Triangle4iSceneBuilderSAH(accel,scene,0);
    else if (g_tri_builder == "sah_spatial" ) builder = BVH4Triangle4iSceneBuilderSpatialSAH(accel,scene,0);
    else if (g_tri_builder == "sah_presplit") builder = BVH4Triangle4iSceneBuilderSAH(accel,scene,MODE_HIGH_QUALITY);
    else if (g_tri_builder == "morton"      ) builder = BVH4Triangle4iSceneBuilderMortonGeneral(accel,scene,0);
    else THROW_RUNTIME_ERROR("unknown builder "+g_tri_builder+" for BVH4<Triangle4i>");

    scene->needVertices = true;
    return new AccelInstance(accel,builder,intersectors);
  }

  void createTriangleMeshTriangle1Morton(TriangleMesh* mesh, AccelData*& accel, Builder*& builder)
  {
    if (mesh->numTimeSteps != 1) THROW_RUNTIME_ERROR("internal error");
    accel = new BVH4(TriangleMeshTriangle1::type,mesh->parent,LeafMode);
    builder = BVH4Triangle1MeshBuilderMortonGeneral(accel,mesh,LeafMode);
  } 

  void createTriangleMeshTriangle1(TriangleMesh* mesh, AccelData*& accel, Builder*& builder)
  {
    if (mesh->numTimeSteps != 1) THROW_RUNTIME_ERROR("internal error");
    accel = new BVH4(TriangleMeshTriangle1::type,mesh->parent,LeafMode);
    switch (mesh->flags) {
    case RTC_GEOMETRY_STATIC:     builder = BVH4Triangle1MeshBuilderSAH(accel,mesh,LeafMode); break;
    case RTC_GEOMETRY_DEFORMABLE: builder = BVH4Triangle1MeshRefitSAH(accel,mesh,LeafMode); break;
    case RTC_GEOMETRY_DYNAMIC:    builder = BVH4Triangle1MeshBuilderMortonGeneral(accel,mesh,LeafMode); break;
    default: THROW_RUNTIME_ERROR("internal error"); 
    }
  } 

  void createTriangleMeshTriangle4(TriangleMesh* mesh, AccelData*& accel, Builder*& builder)
  {
    if (mesh->numTimeSteps != 1) THROW_RUNTIME_ERROR("internal error");
    accel = new BVH4(TriangleMeshTriangle4::type,mesh->parent,LeafMode);
    switch (mesh->flags) {
    case RTC_GEOMETRY_STATIC:     builder = BVH4Triangle4MeshBuilderSAH(accel,mesh,LeafMode); break;
    case RTC_GEOMETRY_DEFORMABLE: builder = BVH4Triangle4MeshRefitSAH(accel,mesh,LeafMode); break;
    case RTC_GEOMETRY_DYNAMIC:    builder = BVH4Triangle4MeshBuilderMortonGeneral(accel,mesh,LeafMode); break;
    default: THROW_RUNTIME_ERROR("internal error"); 
    }
  } 

  void createTriangleMeshTriangle1v(TriangleMesh* mesh, AccelData*& accel, Builder*& builder)
  {
    if (mesh->numTimeSteps != 1) THROW_RUNTIME_ERROR("internal error");
    accel = new BVH4(TriangleMeshTriangle1v::type,mesh->parent,LeafMode);
    switch (mesh->flags) {
    case RTC_GEOMETRY_STATIC:     builder = BVH4Triangle1vMeshBuilderSAH(accel,mesh,LeafMode); break;
    case RTC_GEOMETRY_DEFORMABLE: builder = BVH4Triangle1vMeshRefitSAH  (accel,mesh,LeafMode); break;
    case RTC_GEOMETRY_DYNAMIC:    builder = BVH4Triangle1vMeshBuilderMortonGeneral(accel,mesh,LeafMode); break;
    default: THROW_RUNTIME_ERROR("internal error"); 
    }
  } 

  void createTriangleMeshTriangle4v(TriangleMesh* mesh, AccelData*& accel, Builder*& builder)
  {
    if (mesh->numTimeSteps != 1) THROW_RUNTIME_ERROR("internal error");
    accel = new BVH4(TriangleMeshTriangle4v::type,mesh->parent,LeafMode);
    switch (mesh->flags) {
    case RTC_GEOMETRY_STATIC:     builder = BVH4Triangle4vMeshBuilderSAH(accel,mesh,LeafMode); break;
    case RTC_GEOMETRY_DEFORMABLE: builder = BVH4Triangle4vMeshRefitSAH(accel,mesh,LeafMode); break;
    case RTC_GEOMETRY_DYNAMIC:    builder = BVH4Triangle4vMeshBuilderMortonGeneral(accel,mesh,LeafMode); break;
    default: THROW_RUNTIME_ERROR("internal error"); 
    }
  } 

  void createTriangleMeshTriangle4i(TriangleMesh* mesh, AccelData*& accel, Builder*& builder)
  {
    if (mesh->numTimeSteps != 1) THROW_RUNTIME_ERROR("internal error");
    accel = new BVH4(TriangleMeshTriangle4i::type,mesh->parent,LeafMode);
    switch (mesh->flags) {
    case RTC_GEOMETRY_STATIC:     builder = BVH4Triangle4iMeshBuilderSAH(accel,mesh,LeafMode); break;
    case RTC_GEOMETRY_DEFORMABLE: builder = BVH4Triangle4iMeshRefitSAH(accel,mesh,LeafMode); break;
    case RTC_GEOMETRY_DYNAMIC:    builder = BVH4Triangle4iMeshBuilderMortonGeneral(accel,mesh,LeafMode); break;
    default: THROW_RUNTIME_ERROR("internal error"); 
    }
  } 

  Accel* BVH4::BVH4BVH4Triangle1Morton(Scene* scene)
  {
    BVH4* accel = new BVH4(TriangleMeshTriangle1::type,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle1Intersectors(accel);
    Builder* builder = BVH4BuilderTwoLevelSAH(accel,scene,&createTriangleMeshTriangle1Morton);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4BVH4Triangle1ObjectSplit(Scene* scene)
  {
    BVH4* accel = new BVH4(TriangleMeshTriangle1::type,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle1Intersectors(accel);
    Builder* builder = BVH4BuilderTwoLevelSAH(accel,scene,&createTriangleMeshTriangle1);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4BVH4Triangle4ObjectSplit(Scene* scene)
  {
    BVH4* accel = new BVH4(TriangleMeshTriangle4::type,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle4IntersectorsHybrid(accel);
    Builder* builder = BVH4BuilderTwoLevelSAH(accel,scene,&createTriangleMeshTriangle4);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4BVH4Triangle1vObjectSplit(Scene* scene)
  {
    BVH4* accel = new BVH4(TriangleMeshTriangle1v::type,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle1vIntersectors(accel);
    Builder* builder = BVH4BuilderTwoLevelSAH(accel,scene,&createTriangleMeshTriangle1v);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4BVH4Triangle4vObjectSplit(Scene* scene)
  {
    BVH4* accel = new BVH4(TriangleMeshTriangle4v::type,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle4vIntersectorsHybrid(accel);
    Builder* builder = BVH4BuilderTwoLevelSAH(accel,scene,&createTriangleMeshTriangle4v);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4BVH4Triangle4iObjectSplit(Scene* scene)
  {
    BVH4* accel = new BVH4(TriangleMeshTriangle4i::type,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle4iIntersectors(accel);
    Builder* builder = BVH4BuilderTwoLevelSAH(accel,scene,&createTriangleMeshTriangle4i);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4Triangle1SpatialSplit(Scene* scene)
  {
    BVH4* accel = new BVH4(Triangle1Type::type,scene,LeafMode);
    Builder* builder = BVH4Triangle1SceneBuilderSpatialSAH(accel,scene,0); 
    Accel::Intersectors intersectors = BVH4Triangle1Intersectors(accel);
    return new AccelInstance(accel,builder,intersectors);
  }
  
  Accel* BVH4::BVH4Triangle4SpatialSplit(Scene* scene)
  {
    BVH4* accel = new BVH4(Triangle4Type::type,scene,LeafMode);
    Builder* builder = BVH4Triangle4SceneBuilderSpatialSAH(accel,scene,0); 
    Accel::Intersectors intersectors = BVH4Triangle4IntersectorsHybrid(accel);
    return new AccelInstance(accel,builder,intersectors);
  }

#if defined (__TARGET_AVX__)
  Accel* BVH4::BVH4Triangle8SpatialSplit(Scene* scene)
  {
    BVH4* accel = new BVH4(Triangle8Type::type,scene,LeafMode);
    Builder* builder = BVH4Triangle8SceneBuilderSpatialSAH(accel,scene,0);
    Accel::Intersectors intersectors = BVH4Triangle8IntersectorsHybrid(accel);
    return new AccelInstance(accel,builder,intersectors);
  }
#endif

  Accel* BVH4::BVH4Triangle1ObjectSplit(Scene* scene)
  {
    BVH4* accel = new BVH4(Triangle1Type::type,scene,LeafMode);
    Builder* builder = BVH4Triangle1SceneBuilderSAH(accel,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle1Intersectors(accel);
    return new AccelInstance(accel,builder,intersectors);
  }
  
  Accel* BVH4::BVH4Triangle4ObjectSplit(Scene* scene)
  {
    BVH4* accel = new BVH4(Triangle4Type::type,scene,LeafMode);
    Builder* builder = BVH4Triangle4SceneBuilderSAH(accel,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle4IntersectorsHybrid(accel);
    return new AccelInstance(accel,builder,intersectors);
  }

#if defined (__TARGET_AVX__)
  Accel* BVH4::BVH4Triangle8ObjectSplit(Scene* scene)
  {
    BVH4* accel = new BVH4(Triangle8Type::type,scene,LeafMode);
    Builder* builder = BVH4Triangle8SceneBuilderSAH(accel,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle8IntersectorsHybrid(accel);
    return new AccelInstance(accel,builder,intersectors);
  }
#endif

  Accel* BVH4::BVH4Triangle1vObjectSplit(Scene* scene)
  {
    BVH4* accel = new BVH4(Triangle1vType::type,scene,LeafMode);
    Builder* builder = BVH4Triangle1vSceneBuilderSAH(accel,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle1vIntersectors(accel);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4Triangle4vObjectSplit(Scene* scene)
  {
    BVH4* accel = new BVH4(Triangle4vType::type,scene,LeafMode);
    Builder* builder = BVH4Triangle4vSceneBuilderSAH(accel,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle4vIntersectorsHybrid(accel);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4Triangle4iObjectSplit(Scene* scene)
  {
    BVH4* accel = new BVH4(Triangle4iType::type,scene,LeafMode);
    Builder* builder = BVH4Triangle4iSceneBuilderSAH(accel,scene,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle4iIntersectors(accel);
    scene->needVertices = true;
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4SubdivPatch1(Scene* scene)
  {
    BVH4* accel = new BVH4(SubdivPatch1::type,scene,LeafMode);
    Accel::Intersectors intersectors;
    intersectors.ptr = accel; 
    intersectors.intersector1 = BVH4Subdivpatch1Intersector1;
    intersectors.intersector4 = BVH4Subdivpatch1Intersector4;
    intersectors.intersector8 = BVH4Subdivpatch1Intersector8;
    intersectors.intersector16 = NULL;
    Builder* builder = BVH4SubdivPatch1BuilderBinnedSAH(accel,scene,LeafMode);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4SubdivPatch1Cached(Scene* scene)
  {
    BVH4* accel = new BVH4(SubdivPatch1Cached::type,scene,LeafMode);
    Accel::Intersectors intersectors;
    intersectors.ptr = accel; 
    intersectors.intersector1 = BVH4Subdivpatch1CachedIntersector1;
    intersectors.intersector4 = BVH4Subdivpatch1CachedIntersector4;
    intersectors.intersector8 = BVH4Subdivpatch1CachedIntersector8;
    intersectors.intersector16 = NULL;
    Builder* builder = BVH4SubdivPatch1CachedBuilderBinnedSAH(accel,scene,LeafMode);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4SubdivGrid(Scene* scene)
  {
    BVH4* accel = new BVH4(PrimitiveType2::type,scene,LeafMode); // FIXME: type
    Accel::Intersectors intersectors;
    intersectors.ptr = accel; 
    intersectors.intersector1 = BVH4GridIntersector1;
    intersectors.intersector4 = BVH4GridIntersector4;
    intersectors.intersector8 = BVH4GridIntersector8;
    intersectors.intersector16 = NULL;
    Builder* builder = BVH4SubdivGridBuilderBinnedSAH(accel,scene,LeafMode);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4SubdivGridEager(Scene* scene)
  {
    BVH4* accel = new BVH4(PrimitiveType2::type,scene,LeafMode); // FIXME: type
    Accel::Intersectors intersectors;
    intersectors.ptr = accel; 
    intersectors.intersector1 = BVH4GridIntersector1;
    intersectors.intersector4 = BVH4GridIntersector4;
    intersectors.intersector8 = BVH4GridIntersector8;
    intersectors.intersector16 = NULL;
    Builder* builder = BVH4SubdivGridEagerBuilderBinnedSAH(accel,scene,LeafMode);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4SubdivGridLazy(Scene* scene)
  {
    BVH4* accel = new BVH4(PrimitiveType2::type,scene,LeafMode); // FIXME: type
    Accel::Intersectors intersectors;
    intersectors.ptr = accel; 
    intersectors.intersector1 = BVH4GridLazyIntersector1;
    intersectors.intersector4 = BVH4GridLazyIntersector4;
    intersectors.intersector8 = BVH4GridLazyIntersector8;
    intersectors.intersector16 = NULL;
    Builder* builder = BVH4SubdivGridLazyBuilderBinnedSAH(accel,scene,LeafMode);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4UserGeometry(Scene* scene)
  {
    BVH4* accel = new BVH4(VirtualAccelObjectType::type,scene,LeafMode);
    Accel::Intersectors intersectors;
    intersectors.ptr = accel; 
    intersectors.intersector1 = BVH4VirtualIntersector1;
    intersectors.intersector4 = BVH4VirtualIntersector4Chunk;
    intersectors.intersector8 = BVH4VirtualIntersector8Chunk;
    intersectors.intersector16 = NULL;
    Builder* builder = BVH4VirtualSceneBuilderSAH(accel,scene,LeafMode);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4Triangle1ObjectSplit(TriangleMesh* mesh)
  {
    BVH4* accel = new BVH4(TriangleMeshTriangle1::type,mesh->parent,LeafMode);
    Builder* builder = BVH4Triangle1MeshBuilderSAH(accel,mesh,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle1Intersectors(accel);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4Triangle4ObjectSplit(TriangleMesh* mesh)
  {
    BVH4* accel = new BVH4(TriangleMeshTriangle4::type,mesh->parent,LeafMode);
    Builder* builder = BVH4Triangle4MeshBuilderSAH(accel,mesh,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle4IntersectorsHybrid(accel);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4Triangle1vObjectSplit(TriangleMesh* mesh)
  {
    BVH4* accel = new BVH4(TriangleMeshTriangle1v::type,mesh->parent,LeafMode);
    Builder* builder = BVH4Triangle1vMeshBuilderSAH(accel,mesh,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle1vIntersectors(accel);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4Triangle4vObjectSplit(TriangleMesh* mesh)
  {
    BVH4* accel = new BVH4(TriangleMeshTriangle4v::type,mesh->parent,LeafMode);
    Builder* builder = BVH4Triangle4vMeshBuilderSAH(accel,mesh,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle4vIntersectorsHybrid(accel);
    return new AccelInstance(accel,builder,intersectors);
  }

  Accel* BVH4::BVH4Triangle4Refit(TriangleMesh* mesh)
  {
    BVH4* accel = new BVH4(TriangleMeshTriangle4::type,mesh->parent,LeafMode);
    Builder* builder = BVH4Triangle4MeshRefitSAH(accel,mesh,LeafMode);
    Accel::Intersectors intersectors = BVH4Triangle4IntersectorsHybrid(accel);
    return new AccelInstance(accel,builder,intersectors);
  }
}

