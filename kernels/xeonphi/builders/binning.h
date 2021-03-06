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

#pragma once

#include "common/primref.h"

namespace embree
{

  struct Split 
  {
    __forceinline void reset()
    {
      dim = -1;
      pos = -1;
      numLeft = -1;
      cost = pos_inf;
    }

    __forceinline Split () 
    {
      reset();
    }

    __forceinline Split (const float default_cost) 
    {
      reset();
      cost = default_cost;
    }

    __forceinline bool   valid() const { return pos != -1; } 
    __forceinline bool invalid() const { return pos == -1; } 

    
    /*! stream output */
    friend std::ostream& operator<<(std::ostream& cout, const Split& split) {
      return cout << "Split { " << 
        "dim = " << split.dim << 
        ", pos = " << split.pos << 
        ", numLeft = " << split.numLeft <<
        ", sah = " << split.cost << "}";
    }

  public:
    int dim;
    int pos;
    int numLeft;
    float cost;
  };

  struct BinMapping {
    mic_f centroidDiagonal_2;
    mic_f centroidBoundsMin_2;
    mic_f scale;
    __forceinline BinMapping(const Centroid_Scene_AABB &bounds) 
    {
      const mic_f centroidMin = broadcast4to16f(&bounds.centroid2.lower);
      const mic_f centroidMax = broadcast4to16f(&bounds.centroid2.upper);
      centroidDiagonal_2  = centroidMax-centroidMin;
      centroidBoundsMin_2 = centroidMin;
      scale               = select(centroidDiagonal_2 != 0.0f,rcp(centroidDiagonal_2) * mic_f(16.0f * 0.99f),mic_f::zero());      
    }

    __forceinline mic_i getBinID(const mic_f &centroid_2) const
    {
      return convert_uint32((centroid_2 - centroidBoundsMin_2)*scale);
    }

    __forceinline mic_m getValidDimMask() const
    {
      return (centroidDiagonal_2 > 0.0f) & (mic_m)0x7;
    }
    
  };

  struct BinPartitionMapping {

    mic_f c;
    mic_f s;
    mic_f bestSplit_f;
    mic_m dim_mask;

     __forceinline BinPartitionMapping(const Split &split, const Centroid_Scene_AABB &bounds) 
    {
      const unsigned int bestSplitDim     = split.dim;
      const unsigned int bestSplit        = split.pos;
      const unsigned int bestSplitNumLeft = split.numLeft;
      const mic_f centroidMin = broadcast4to16f(&bounds.centroid2.lower);
      const mic_f centroidMax = broadcast4to16f(&bounds.centroid2.upper);      
      const mic_f centroidBoundsMin_2 = centroidMin;
      const mic_f centroidDiagonal_2  = centroidMax-centroidMin;
      const mic_f scale = select(centroidDiagonal_2 != 0.0f,rcp(centroidDiagonal_2) * mic_f(16.0f * 0.99f),mic_f::zero());
      c = mic_f(centroidBoundsMin_2[bestSplitDim]);
      s = mic_f(scale[bestSplitDim]);
      bestSplit_f = mic_f(bestSplit);
      dim_mask = mic_m::shift1[bestSplitDim];      
    }

    __forceinline mic_m lt_split(const mic_f &b_min,
				 const mic_f &b_max) const
    {
      const mic_f centroid_2 = b_min + b_max;
      const mic_f binID = (centroid_2 - c)*s;
      return lt(dim_mask,binID,bestSplit_f);    
    }

    __forceinline mic_m ge_split(const mic_f &b_min,
				 const mic_f &b_max) const
    {
      const mic_f centroid_2 = b_min + b_max;
      const mic_f binID = (centroid_2 - c)*s;
      return ge(dim_mask,binID,bestSplit_f);    
    }
    
  };

  

  __aligned(64) static const int identity[16]         = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
  __aligned(64) static const int reverse_identity[16] = { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0 };

  __forceinline mic_f reverse(const mic_f &a) 
  {
    return _mm512_permutev_ps(load16i(reverse_identity),a);
  }
  
  __forceinline mic_f prefix_area_rl(const mic_f min_x,
				     const mic_f min_y,
				     const mic_f min_z,
				     const mic_f max_x,
				     const mic_f max_y,
				     const mic_f max_z)
  {
    const mic_f r_min_x = prefix_min(reverse(min_x));
    const mic_f r_min_y = prefix_min(reverse(min_y));
    const mic_f r_min_z = prefix_min(reverse(min_z));
    const mic_f r_max_x = prefix_max(reverse(max_x));
    const mic_f r_max_y = prefix_max(reverse(max_y));
    const mic_f r_max_z = prefix_max(reverse(max_z));
    
    const mic_f dx = r_max_x - r_min_x;
    const mic_f dy = r_max_y - r_min_y;
    const mic_f dz = r_max_z - r_min_z;
    
    const mic_f area_rl = (dx*dy+dx*dz+dy*dz) * mic_f(2.0f);
    return reverse(shl1_zero_extend(area_rl));
  }

  __forceinline mic_f prefix_area_lr(const mic_f min_x,
				     const mic_f min_y,
				     const mic_f min_z,
				     const mic_f max_x,
				     const mic_f max_y,
				     const mic_f max_z)
  {
    const mic_f r_min_x = prefix_min(min_x);
    const mic_f r_min_y = prefix_min(min_y);
    const mic_f r_min_z = prefix_min(min_z);
    const mic_f r_max_x = prefix_max(max_x);
    const mic_f r_max_y = prefix_max(max_y);
    const mic_f r_max_z = prefix_max(max_z);
    
    const mic_f dx = r_max_x - r_min_x;
    const mic_f dy = r_max_y - r_min_y;
    const mic_f dz = r_max_z - r_min_z;
  
    const mic_f area_lr = (dx*dy+dx*dz+dy*dz) * mic_f(2.0f);
    return area_lr;
  }


  __forceinline mic_i prefix_count(const mic_i c)
  {
    return prefix_sum(c);
  }


  __forceinline Split getBestSplit(const BuildRecord& current,
				   const mic_f leftArea[3],
				   const mic_f rightArea[3],
				   const mic_i leftNum[3],
				   const mic_m &m_dim)
  {
    const unsigned int items        = current.items();
    const float voxelArea           = area(current.bounds.geometry);
    //const mic_f centroidMin         = broadcast4to16f(&current.bounds.centroid2.lower);
    //const mic_f centroidMax         = broadcast4to16f(&current.bounds.centroid2.upper);
    //const mic_f centroidDiagonal_2  = centroidMax-centroidMin;

    Split split( items * voxelArea );
 
    long dim = -1;
    while((dim = bitscan64(dim,m_dim)) != BITSCAN_NO_BIT_SET_64) 
      {	    
	//assert(centroidDiagonal_2[dim] > 0.0f);

	const mic_f rArea   = rightArea[dim]; // bin16.prefix_area_rl(dim);
	const mic_f lArea   = leftArea[dim];  // bin16.prefix_area_lr(dim);      
	const mic_i lnum    = leftNum[dim];   // bin16.prefix_count(dim);

	const mic_i rnum    = mic_i(items) - lnum;
	const mic_i lblocks = (lnum + mic_i(3)) >> 2;
	const mic_i rblocks = (rnum + mic_i(3)) >> 2;
	const mic_m m_lnum  = lnum == 0;
	const mic_m m_rnum  = rnum == 0;
	const mic_f cost    = select(m_lnum|m_rnum,mic_f::inf(),lArea * mic_f(lblocks) + rArea * mic_f(rblocks) + voxelArea );

	if (lt(cost,mic_f(split.cost)))
	  {

	    const mic_f min_cost    = vreduce_min(cost); 
	    const mic_m m_pos       = min_cost == cost;
	    const unsigned long pos = bitscan64(m_pos);	    

	    assert(pos < 15);

	    if (pos < 15)
	      {
		split.cost    = cost[pos];
		split.pos     = pos+1;
		split.dim     = dim;	    
		split.numLeft = lnum[pos];
	      }
	  }
      };
    return split;
  }


  template<class Primitive>
  __forceinline void fastbin(const Primitive * __restrict__ const aabb,
			     const unsigned int thread_start,
			     const unsigned int thread_end,
			     const BinMapping &mapping,
			     mic_f lArea[3],
			     mic_f rArea[3],
			     mic_i lNum[3])
  {

    const mic_f init_min = mic_f::inf();
    const mic_f init_max = mic_f::minus_inf();
    const mic_i zero     = mic_i::zero();

    mic_f min_x0,min_x1,min_x2;
    mic_f min_y0,min_y1,min_y2;
    mic_f min_z0,min_z1,min_z2;
    mic_f max_x0,max_x1,max_x2;
    mic_f max_y0,max_y1,max_y2;
    mic_f max_z0,max_z1,max_z2;
    mic_i count0,count1,count2;

    min_x0 = init_min;
    min_x1 = init_min;
    min_x2 = init_min;
    min_y0 = init_min;
    min_y1 = init_min;
    min_y2 = init_min;
    min_z0 = init_min;
    min_z1 = init_min;
    min_z2 = init_min;

    max_x0 = init_max;
    max_x1 = init_max;
    max_x2 = init_max;
    max_y0 = init_max;
    max_y1 = init_max;
    max_y2 = init_max;
    max_z0 = init_max;
    max_z1 = init_max;
    max_z2 = init_max;

    count0 = zero;
    count1 = zero;
    count2 = zero;

    unsigned int start = thread_start;
    unsigned int end   = thread_end;

    if (unlikely(start % 2 != 0))
      {
	const mic2f bounds = aabb[start].getBounds();
	const mic_f b_min = bounds.x;
	const mic_f b_max = bounds.y;

	const mic_f centroid_2 = b_min + b_max; 
	const mic_i binID = mapping.getBinID(centroid_2); // convert_uint32((centroid_2 - centroidBoundsMin_2)*scale);
	// ------------------------------------------------------------------------      
	assert(0 <= binID[0] && binID[0] < 16);
	assert(0 <= binID[1] && binID[1] < 16);
	assert(0 <= binID[2] && binID[2] < 16);
	// ------------------------------------------------------------------------      
	const mic_i id = load16i(identity);
	const mic_m m_update_x = eq(id,swAAAA(binID));
	const mic_m m_update_y = eq(id,swBBBB(binID));
	const mic_m m_update_z = eq(id,swCCCC(binID));
	// ------------------------------------------------------------------------      
	min_x0 = mask_min(m_update_x,min_x0,min_x0,swAAAA(b_min));
	min_y0 = mask_min(m_update_x,min_y0,min_y0,swBBBB(b_min));
	min_z0 = mask_min(m_update_x,min_z0,min_z0,swCCCC(b_min));
	// ------------------------------------------------------------------------      
	max_x0 = mask_max(m_update_x,max_x0,max_x0,swAAAA(b_max));
	max_y0 = mask_max(m_update_x,max_y0,max_y0,swBBBB(b_max));
	max_z0 = mask_max(m_update_x,max_z0,max_z0,swCCCC(b_max));
	// ------------------------------------------------------------------------
	min_x1 = mask_min(m_update_y,min_x1,min_x1,swAAAA(b_min));
	min_y1 = mask_min(m_update_y,min_y1,min_y1,swBBBB(b_min));
	min_z1 = mask_min(m_update_y,min_z1,min_z1,swCCCC(b_min));      
	// ------------------------------------------------------------------------      
	max_x1 = mask_max(m_update_y,max_x1,max_x1,swAAAA(b_max));
	max_y1 = mask_max(m_update_y,max_y1,max_y1,swBBBB(b_max));
	max_z1 = mask_max(m_update_y,max_z1,max_z1,swCCCC(b_max));
	// ------------------------------------------------------------------------
	min_x2 = mask_min(m_update_z,min_x2,min_x2,swAAAA(b_min));
	min_y2 = mask_min(m_update_z,min_y2,min_y2,swBBBB(b_min));
	min_z2 = mask_min(m_update_z,min_z2,min_z2,swCCCC(b_min));
	// ------------------------------------------------------------------------      
	max_x2 = mask_max(m_update_z,max_x2,max_x2,swAAAA(b_max));
	max_y2 = mask_max(m_update_z,max_y2,max_y2,swBBBB(b_max));
	max_z2 = mask_max(m_update_z,max_z2,max_z2,swCCCC(b_max));
	// ------------------------------------------------------------------------
	count0 = mask_add(m_update_x,count0,count0,mic_i::one());
	count1 = mask_add(m_update_y,count1,count1,mic_i::one());
	count2 = mask_add(m_update_z,count2,count2,mic_i::one());      
	start++;	
      }
    assert(start % 2 == 0);

    if (unlikely(end % 2 != 0))
      {
	const mic2f bounds = aabb[end-1].getBounds();
	const mic_f b_min = bounds.x;
	const mic_f b_max = bounds.y;

	const mic_f centroid_2 = b_min + b_max; 
	const mic_i binID = mapping.getBinID(centroid_2); // const mic_i binID = convert_uint32((centroid_2 - centroidBoundsMin_2)*scale);
	// ------------------------------------------------------------------------      
	assert(0 <= binID[0] && binID[0] < 16);
	assert(0 <= binID[1] && binID[1] < 16);
	assert(0 <= binID[2] && binID[2] < 16);
	// ------------------------------------------------------------------------      
	const mic_i id = load16i(identity);
	const mic_m m_update_x = eq(id,swAAAA(binID));
	const mic_m m_update_y = eq(id,swBBBB(binID));
	const mic_m m_update_z = eq(id,swCCCC(binID));
	// ------------------------------------------------------------------------      
	min_x0 = mask_min(m_update_x,min_x0,min_x0,swAAAA(b_min));
	min_y0 = mask_min(m_update_x,min_y0,min_y0,swBBBB(b_min));
	min_z0 = mask_min(m_update_x,min_z0,min_z0,swCCCC(b_min));
	// ------------------------------------------------------------------------      
	max_x0 = mask_max(m_update_x,max_x0,max_x0,swAAAA(b_max));
	max_y0 = mask_max(m_update_x,max_y0,max_y0,swBBBB(b_max));
	max_z0 = mask_max(m_update_x,max_z0,max_z0,swCCCC(b_max));
	// ------------------------------------------------------------------------
	min_x1 = mask_min(m_update_y,min_x1,min_x1,swAAAA(b_min));
	min_y1 = mask_min(m_update_y,min_y1,min_y1,swBBBB(b_min));
	min_z1 = mask_min(m_update_y,min_z1,min_z1,swCCCC(b_min));      
	// ------------------------------------------------------------------------      
	max_x1 = mask_max(m_update_y,max_x1,max_x1,swAAAA(b_max));
	max_y1 = mask_max(m_update_y,max_y1,max_y1,swBBBB(b_max));
	max_z1 = mask_max(m_update_y,max_z1,max_z1,swCCCC(b_max));
	// ------------------------------------------------------------------------
	min_x2 = mask_min(m_update_z,min_x2,min_x2,swAAAA(b_min));
	min_y2 = mask_min(m_update_z,min_y2,min_y2,swBBBB(b_min));
	min_z2 = mask_min(m_update_z,min_z2,min_z2,swCCCC(b_min));
	// ------------------------------------------------------------------------      
	max_x2 = mask_max(m_update_z,max_x2,max_x2,swAAAA(b_max));
	max_y2 = mask_max(m_update_z,max_y2,max_y2,swBBBB(b_max));
	max_z2 = mask_max(m_update_z,max_z2,max_z2,swCCCC(b_max));
	// ------------------------------------------------------------------------
	count0 = mask_add(m_update_x,count0,count0,mic_i::one());
	count1 = mask_add(m_update_y,count1,count1,mic_i::one());
	count2 = mask_add(m_update_z,count2,count2,mic_i::one());      
	end--;	
      }
    assert(end % 2 == 0);

    const Primitive * __restrict__ aptr = aabb + start;

    prefetch<PFHINT_NT>(aptr);
    prefetch<PFHINT_L2>(aptr+2);
    prefetch<PFHINT_L2>(aptr+4);
    prefetch<PFHINT_L2>(aptr+6);
    prefetch<PFHINT_L2>(aptr+8);

    for (size_t j = start;j < end;j+=2,aptr+=2)
      {
	prefetch<PFHINT_L1>(aptr+2);
	prefetch<PFHINT_L2>(aptr+12);
	
#pragma unroll(2)
	for (size_t i=0;i<2;i++)
	  {

	    const mic2f bounds = aptr[i].getBounds();
	    const mic_f b_min  = bounds.x;
	    const mic_f b_max  = bounds.y;

	    const mic_f centroid_2 = b_min + b_max;
	    const mic_i binID = mapping.getBinID(centroid_2); // const mic_i binID = convert_uint32((centroid_2 - centroidBoundsMin_2)*scale);

	    assert(0 <= binID[0] && binID[0] < 16);
	    assert(0 <= binID[1] && binID[1] < 16);
	    assert(0 <= binID[2] && binID[2] < 16);

	    const mic_i id = load16i(identity);
	    const mic_m m_update_x = eq(id,swAAAA(binID));
	    const mic_m m_update_y = eq(id,swBBBB(binID));
	    const mic_m m_update_z = eq(id,swCCCC(binID));

	    min_x0 = mask_min(m_update_x,min_x0,min_x0,swAAAA(b_min));
	    min_y0 = mask_min(m_update_x,min_y0,min_y0,swBBBB(b_min));
	    min_z0 = mask_min(m_update_x,min_z0,min_z0,swCCCC(b_min));
	    // ------------------------------------------------------------------------      
	    max_x0 = mask_max(m_update_x,max_x0,max_x0,swAAAA(b_max));
	    max_y0 = mask_max(m_update_x,max_y0,max_y0,swBBBB(b_max));
	    max_z0 = mask_max(m_update_x,max_z0,max_z0,swCCCC(b_max));
	    // ------------------------------------------------------------------------
	    min_x1 = mask_min(m_update_y,min_x1,min_x1,swAAAA(b_min));
	    min_y1 = mask_min(m_update_y,min_y1,min_y1,swBBBB(b_min));
	    min_z1 = mask_min(m_update_y,min_z1,min_z1,swCCCC(b_min));      
	    // ------------------------------------------------------------------------      
	    max_x1 = mask_max(m_update_y,max_x1,max_x1,swAAAA(b_max));
	    max_y1 = mask_max(m_update_y,max_y1,max_y1,swBBBB(b_max));
	    max_z1 = mask_max(m_update_y,max_z1,max_z1,swCCCC(b_max));
	    // ------------------------------------------------------------------------
	    min_x2 = mask_min(m_update_z,min_x2,min_x2,swAAAA(b_min));
	    min_y2 = mask_min(m_update_z,min_y2,min_y2,swBBBB(b_min));
	    min_z2 = mask_min(m_update_z,min_z2,min_z2,swCCCC(b_min));
	    // ------------------------------------------------------------------------      
	    max_x2 = mask_max(m_update_z,max_x2,max_x2,swAAAA(b_max));
	    max_y2 = mask_max(m_update_z,max_y2,max_y2,swBBBB(b_max));
	    max_z2 = mask_max(m_update_z,max_z2,max_z2,swCCCC(b_max));
	    // ------------------------------------------------------------------------
	    count0 = mask_add(m_update_x,count0,count0,mic_i::one());
	    count1 = mask_add(m_update_y,count1,count1,mic_i::one());
	    count2 = mask_add(m_update_z,count2,count2,mic_i::one());      
	  }
      }

    prefetch<PFHINT_L1EX>(&rArea[0]);
    prefetch<PFHINT_L1EX>(&lArea[0]);
    prefetch<PFHINT_L1EX>(&lNum[0]);
    rArea[0] = prefix_area_rl(min_x0,min_y0,min_z0,max_x0,max_y0,max_z0);
    lArea[0] = prefix_area_lr(min_x0,min_y0,min_z0,max_x0,max_y0,max_z0);
    lNum[0]  = prefix_count(count0);

    prefetch<PFHINT_L1EX>(&rArea[1]);
    prefetch<PFHINT_L1EX>(&lArea[1]);
    prefetch<PFHINT_L1EX>(&lNum[1]);
    rArea[1] = prefix_area_rl(min_x1,min_y1,min_z1,max_x1,max_y1,max_z1);
    lArea[1] = prefix_area_lr(min_x1,min_y1,min_z1,max_x1,max_y1,max_z1);
    lNum[1]  = prefix_count(count1);

    prefetch<PFHINT_L1EX>(&rArea[2]);
    prefetch<PFHINT_L1EX>(&lArea[2]);
    prefetch<PFHINT_L1EX>(&lNum[2]);
    rArea[2] = prefix_area_rl(min_x2,min_y2,min_z2,max_x2,max_y2,max_z2);
    lArea[2] = prefix_area_lr(min_x2,min_y2,min_z2,max_x2,max_y2,max_z2);
    lNum[2]  = prefix_count(count2);
  }



  template<class Primitive>
  __forceinline void fastbin_xfm(const Primitive * __restrict__ const aabb,
				 const mic3f &cmat,
				 const unsigned int thread_start,
				 const unsigned int thread_end,
				 const BinMapping &mapping,
				 mic_f lArea[3],
				 mic_f rArea[3],
				 mic_i lNum[3])
  {

    const mic_f init_min = mic_f::inf();
    const mic_f init_max = mic_f::minus_inf();
    const mic_i zero     = mic_i::zero();

    mic_f min_x0,min_x1,min_x2;
    mic_f min_y0,min_y1,min_y2;
    mic_f min_z0,min_z1,min_z2;
    mic_f max_x0,max_x1,max_x2;
    mic_f max_y0,max_y1,max_y2;
    mic_f max_z0,max_z1,max_z2;
    mic_i count0,count1,count2;

    min_x0 = init_min;
    min_x1 = init_min;
    min_x2 = init_min;
    min_y0 = init_min;
    min_y1 = init_min;
    min_y2 = init_min;
    min_z0 = init_min;
    min_z1 = init_min;
    min_z2 = init_min;

    max_x0 = init_max;
    max_x1 = init_max;
    max_x2 = init_max;
    max_y0 = init_max;
    max_y1 = init_max;
    max_y2 = init_max;
    max_z0 = init_max;
    max_z1 = init_max;
    max_z2 = init_max;

    count0 = zero;
    count1 = zero;
    count2 = zero;

    unsigned int start = thread_start;
    unsigned int end   = thread_end;

    const Primitive * __restrict__ aptr = aabb + start;

    prefetch<PFHINT_NT>(aptr);
    prefetch<PFHINT_L2>(aptr+2);
    prefetch<PFHINT_L2>(aptr+4);
    prefetch<PFHINT_L2>(aptr+6);
    prefetch<PFHINT_L2>(aptr+8);

    const mic_f c0 = cmat.x;
    const mic_f c1 = cmat.y;
    const mic_f c2 = cmat.z;

    for (size_t j = start;j < end;j++,aptr++)
      {
	prefetch<PFHINT_L1>(aptr+2);
	prefetch<PFHINT_L2>(aptr+12);
	
	const mic2f bounds = aptr->getBounds(c0,c1,c2);

	const mic_f b_min  = bounds.x;
	const mic_f b_max  = bounds.y;

	const mic_f centroid_2 = b_min + b_max;
	const mic_i binID_noclamp = mapping.getBinID(centroid_2); // convert_uint32((centroid_2 - centroidBoundsMin_2)*scale);
	const mic_i binID = min(max(binID_noclamp,mic_i::zero()),mic_i(15));
	assert(0 <= binID[0] && binID[0] < 16);
	assert(0 <= binID[1] && binID[1] < 16);
	assert(0 <= binID[2] && binID[2] < 16);

	const mic_i id = load16i(identity);
	const mic_m m_update_x = eq(id,swAAAA(binID));
	const mic_m m_update_y = eq(id,swBBBB(binID));
	const mic_m m_update_z = eq(id,swCCCC(binID));

	min_x0 = mask_min(m_update_x,min_x0,min_x0,swAAAA(b_min));
	min_y0 = mask_min(m_update_x,min_y0,min_y0,swBBBB(b_min));
	min_z0 = mask_min(m_update_x,min_z0,min_z0,swCCCC(b_min));
	// ------------------------------------------------------------------------      
	max_x0 = mask_max(m_update_x,max_x0,max_x0,swAAAA(b_max));
	max_y0 = mask_max(m_update_x,max_y0,max_y0,swBBBB(b_max));
	max_z0 = mask_max(m_update_x,max_z0,max_z0,swCCCC(b_max));
	// ------------------------------------------------------------------------
	min_x1 = mask_min(m_update_y,min_x1,min_x1,swAAAA(b_min));
	min_y1 = mask_min(m_update_y,min_y1,min_y1,swBBBB(b_min));
	min_z1 = mask_min(m_update_y,min_z1,min_z1,swCCCC(b_min));      
	// ------------------------------------------------------------------------      
	max_x1 = mask_max(m_update_y,max_x1,max_x1,swAAAA(b_max));
	max_y1 = mask_max(m_update_y,max_y1,max_y1,swBBBB(b_max));
	max_z1 = mask_max(m_update_y,max_z1,max_z1,swCCCC(b_max));
	// ------------------------------------------------------------------------
	min_x2 = mask_min(m_update_z,min_x2,min_x2,swAAAA(b_min));
	min_y2 = mask_min(m_update_z,min_y2,min_y2,swBBBB(b_min));
	min_z2 = mask_min(m_update_z,min_z2,min_z2,swCCCC(b_min));
	// ------------------------------------------------------------------------      
	max_x2 = mask_max(m_update_z,max_x2,max_x2,swAAAA(b_max));
	max_y2 = mask_max(m_update_z,max_y2,max_y2,swBBBB(b_max));
	max_z2 = mask_max(m_update_z,max_z2,max_z2,swCCCC(b_max));
	// ------------------------------------------------------------------------
	count0 = mask_add(m_update_x,count0,count0,mic_i::one());
	count1 = mask_add(m_update_y,count1,count1,mic_i::one());
	count2 = mask_add(m_update_z,count2,count2,mic_i::one());      
      }

    prefetch<PFHINT_L1EX>(&rArea[0]);
    prefetch<PFHINT_L1EX>(&lArea[0]);
    prefetch<PFHINT_L1EX>(&lNum[0]);
    rArea[0] = prefix_area_rl(min_x0,min_y0,min_z0,max_x0,max_y0,max_z0);
    lArea[0] = prefix_area_lr(min_x0,min_y0,min_z0,max_x0,max_y0,max_z0);
    lNum[0]  = prefix_count(count0);

    prefetch<PFHINT_L1EX>(&rArea[1]);
    prefetch<PFHINT_L1EX>(&lArea[1]);
    prefetch<PFHINT_L1EX>(&lNum[1]);
    rArea[1] = prefix_area_rl(min_x1,min_y1,min_z1,max_x1,max_y1,max_z1);
    lArea[1] = prefix_area_lr(min_x1,min_y1,min_z1,max_x1,max_y1,max_z1);
    lNum[1]  = prefix_count(count1);

    prefetch<PFHINT_L1EX>(&rArea[2]);
    prefetch<PFHINT_L1EX>(&lArea[2]);
    prefetch<PFHINT_L1EX>(&lNum[2]);
    rArea[2] = prefix_area_rl(min_x2,min_y2,min_z2,max_x2,max_y2,max_z2);
    lArea[2] = prefix_area_lr(min_x2,min_y2,min_z2,max_x2,max_y2,max_z2);
    lNum[2]  = prefix_count(count2);
  }


  template<unsigned int DISTANCE, class Primitive>
    __forceinline unsigned int partitionPrimitives_xfm(Primitive *__restrict__ aabb,
						       const mic3f &cmat,
						       const unsigned int begin,
						       const unsigned int end,
						       const BinPartitionMapping &mapping,
						       Centroid_Scene_AABB & local_left,
						       Centroid_Scene_AABB & local_right)
    {
      assert(begin <= end);

      Primitive *__restrict__ l = aabb + begin;
      Primitive *__restrict__ r = aabb + end - 1;

      CentroidGeometryAABB leftReduction;
      CentroidGeometryAABB rightReduction;
      leftReduction.reset();
      rightReduction.reset();

      const mic_f c0 = cmat.x;
      const mic_f c1 = cmat.y;
      const mic_f c2 = cmat.z;

      while(1)
	{
	  while (likely(l <= r)) 
	    {
	      
	      const mic2f bounds = l->getBounds(c0,c1,c2);
	      const mic_f b_min  = bounds.x;
	      const mic_f b_max  = bounds.y;

	      prefetch<PFHINT_L1EX>(((char*)l)+4*64);
	      if (unlikely(mapping.ge_split(b_min,b_max))) break;
	      prefetch<PFHINT_L2EX>(((char*)l)+20*64);
 
	      leftReduction.extend(b_min,b_max);
	      ++l;
	    }
	  while (likely(l <= r)) 
	    {
	      const mic2f bounds = r->getBounds(c0,c1,c2);
	      const mic_f b_min  = bounds.x;
	      const mic_f b_max  = bounds.y;

	      prefetch<PFHINT_L1EX>(((char*)r)-4*64);	  
	      if (unlikely(mapping.lt_split(b_min,b_max))) break;
	      prefetch<PFHINT_L2EX>(((char*)r)-20*64);	  
	      rightReduction.extend(b_min,b_max);
	      --r;
	    }

	  if (unlikely(r<l)) {
	    break;
	  }

	  rightReduction.extend(l->getBounds());
	  leftReduction.extend(r->getBounds());

	  xchg(*l,*r);
	  l++; r--;
	}

      local_left  = Centroid_Scene_AABB( leftReduction );
      local_right = Centroid_Scene_AABB( rightReduction );

      assert( aabb + begin <= l && l <= aabb + end);
      assert( aabb + begin <= r && r <= aabb + end);

      return l - (aabb + begin);
    }


  class __aligned(64) Bin16
  {
  public:
    mic_f min_x[3];
    mic_f min_y[3];
    mic_f min_z[3];
    mic_f max_x[3];
    mic_f max_y[3];
    mic_f max_z[3];
    mic_i count[3];
    mic_i thread_count[3];

    Bin16() {}

    __forceinline void prefetchL2()
    {
      const unsigned int size = sizeof(Bin16);
#pragma unroll(8)
      for (size_t i=0;i<size;i+=64)
	prefetch<PFHINT_L2>(((char*)this) + i);
	
    }
    __forceinline void prefetchL2EX()
    {
      prefetch<PFHINT_L2EX>(&min_x[0]);
      prefetch<PFHINT_L2EX>(&min_x[1]);
      prefetch<PFHINT_L2EX>(&min_x[2]);

      prefetch<PFHINT_L2EX>(&min_y[0]);
      prefetch<PFHINT_L2EX>(&min_y[1]);
      prefetch<PFHINT_L2EX>(&min_y[2]);

      prefetch<PFHINT_L2EX>(&min_z[0]);
      prefetch<PFHINT_L2EX>(&min_z[1]);
      prefetch<PFHINT_L2EX>(&min_z[2]);

      prefetch<PFHINT_L2EX>(&max_x[0]);
      prefetch<PFHINT_L2EX>(&max_x[1]);
      prefetch<PFHINT_L2EX>(&max_x[2]);

      prefetch<PFHINT_L2EX>(&max_y[0]);
      prefetch<PFHINT_L2EX>(&max_y[1]);
      prefetch<PFHINT_L2EX>(&max_y[2]);

      prefetch<PFHINT_L2EX>(&max_z[0]);
      prefetch<PFHINT_L2EX>(&max_z[1]);
      prefetch<PFHINT_L2EX>(&max_z[2]);

      prefetch<PFHINT_L2EX>(&count[0]);
      prefetch<PFHINT_L2EX>(&count[1]);
      prefetch<PFHINT_L2EX>(&count[2]);

      prefetch<PFHINT_L2EX>(&thread_count[0]);
      prefetch<PFHINT_L2EX>(&thread_count[1]);
      prefetch<PFHINT_L2EX>(&thread_count[2]);
    }


    __forceinline void reset()
    {
      const mic_f init_min = mic_f::inf();
      const mic_f init_max = mic_f::minus_inf();
      const mic_i zero     = mic_i::zero();

      min_x[0] = init_min;
      min_x[1] = init_min;
      min_x[2] = init_min;

      min_y[0] = init_min;
      min_y[1] = init_min;
      min_y[2] = init_min;

      min_z[0] = init_min;
      min_z[1] = init_min;
      min_z[2] = init_min;

      max_x[0] = init_max;
      max_x[1] = init_max;
      max_x[2] = init_max;

      max_y[0] = init_max;
      max_y[1] = init_max;
      max_y[2] = init_max;

      max_z[0] = init_max;
      max_z[1] = init_max;
      max_z[2] = init_max;

      count[0] = zero;
      count[1] = zero;
      count[2] = zero;
    }


    __forceinline void merge(const Bin16& b)
    {
#pragma unroll(3)
      for (unsigned int i=0;i<3;i++)
	{
	  min_x[i] = min(min_x[i],b.min_x[i]);
	  min_y[i] = min(min_y[i],b.min_y[i]);
	  min_z[i] = min(min_z[i],b.min_z[i]);

	  max_x[i] = max(max_x[i],b.max_x[i]);
	  max_y[i] = max(max_y[i],b.max_y[i]);
	  max_z[i] = max(max_z[i],b.max_z[i]);

	  count[i] += b.count[i];
	}
    } 


    __forceinline Split bestSplit(const BuildRecord& current,const mic_m &m_dim) const
    {
      mic_f lArea[3];
      mic_f rArea[3];
      mic_i leftNum[3];

      long dim = -1;
      while((dim = bitscan64(dim,m_dim)) != BITSCAN_NO_BIT_SET_64) 
	{	
	  rArea[dim]   = prefix_area_rl(min_x[dim],min_y[dim],min_z[dim],
					max_x[dim],max_y[dim],max_z[dim]);
	  lArea[dim]   = prefix_area_lr(min_x[dim],min_y[dim],min_z[dim],
					max_x[dim],max_y[dim],max_z[dim]);
	  leftNum[dim] = prefix_count(count[dim]);
	}    

      return getBestSplit(current,lArea,rArea,leftNum,m_dim);
      
    }


  };

  __forceinline bool operator==(const Bin16 &a, const Bin16 &b) { 
    mic_m mask = 0xffff;
#pragma unroll(3)
    for (unsigned int i=0;i<3;i++)
      {
	mask &= eq(a.min_x[i],b.min_x[i]);
	mask &= eq(a.min_y[i],b.min_y[i]);
	mask &= eq(a.min_z[i],b.min_z[i]);

	mask &= eq(a.max_x[i],b.max_x[i]);
	mask &= eq(a.max_y[i],b.max_y[i]);
	mask &= eq(a.max_z[i],b.max_z[i]);

	mask &= eq(a.count[i],b.count[i]);
      }
    return mask == (mic_m)0xffff;
  };

  __forceinline bool operator!=(const Bin16 &a, const Bin16 &b) { 
    return !(a==b);
  }

  __forceinline std::ostream &operator<<(std::ostream &o, const Bin16 &v)
  {
#pragma unroll(3)
    for (unsigned int i=0;i<3;i++)
      {
	DBG_PRINT(v.min_x[i]);
	DBG_PRINT(v.min_y[i]);
	DBG_PRINT(v.min_z[i]);

	DBG_PRINT(v.max_x[i]);
	DBG_PRINT(v.max_y[i]);
	DBG_PRINT(v.max_z[i]);

	DBG_PRINT(v.count[i]);
      }

    return o;
  }


  template<class Primitive>
  __forceinline void fastbin(const Primitive * __restrict__ const aabb,
			     const unsigned int thread_start,
			     const unsigned int thread_end,
			     const BinMapping &mapping,
			     Bin16 &bin16)
  {
    const mic_f init_min = mic_f::inf();
    const mic_f init_max = mic_f::minus_inf();
    const mic_i zero     = mic_i::zero();

    mic_f min_x0,min_x1,min_x2;
    mic_f min_y0,min_y1,min_y2;
    mic_f min_z0,min_z1,min_z2;
    mic_f max_x0,max_x1,max_x2;
    mic_f max_y0,max_y1,max_y2;
    mic_f max_z0,max_z1,max_z2;
    mic_i count0,count1,count2;

    min_x0 = init_min;
    min_x1 = init_min;
    min_x2 = init_min;
    min_y0 = init_min;
    min_y1 = init_min;
    min_y2 = init_min;
    min_z0 = init_min;
    min_z1 = init_min;
    min_z2 = init_min;

    max_x0 = init_max;
    max_x1 = init_max;
    max_x2 = init_max;
    max_y0 = init_max;
    max_y1 = init_max;
    max_y2 = init_max;
    max_z0 = init_max;
    max_z1 = init_max;
    max_z2 = init_max;

    count0 = zero;
    count1 = zero;
    count2 = zero;

    unsigned int start = thread_start;
    unsigned int end   = thread_end;

    const Primitive * __restrict__ aptr = aabb + start;

    prefetch<PFHINT_NT>(aptr);
    prefetch<PFHINT_L2>(aptr+2);
    prefetch<PFHINT_L2>(aptr+4);
    prefetch<PFHINT_L2>(aptr+6);
    prefetch<PFHINT_L2>(aptr+8);

    for (size_t j = start;j < end;j++,aptr++)
      {
	prefetch<PFHINT_NT>(aptr+2);
	prefetch<PFHINT_L2>(aptr+12);

	const mic2f bounds = aptr->getBounds();
	const mic_f b_min  = bounds.x;
	const mic_f b_max  = bounds.y;

	const mic_f centroid_2 = b_min + b_max; 
	const mic_i binID = mapping.getBinID(centroid_2); // convert_uint32((centroid_2 - centroidBoundsMin_2)*scale);

	assert(0 <= binID[0] && binID[0] < 16); 
	assert(0 <= binID[1] && binID[1] < 16); 
	assert(0 <= binID[2] && binID[2] < 16); 

	const mic_i id = load16i(identity);
	const mic_m m_update_x = eq(id,swAAAA(binID));
	const mic_m m_update_y = eq(id,swBBBB(binID));
	const mic_m m_update_z = eq(id,swCCCC(binID));

	min_x0 = mask_min(m_update_x,min_x0,min_x0,swAAAA(b_min));
	min_y0 = mask_min(m_update_x,min_y0,min_y0,swBBBB(b_min));
	min_z0 = mask_min(m_update_x,min_z0,min_z0,swCCCC(b_min));
	// ------------------------------------------------------------------------      
	max_x0 = mask_max(m_update_x,max_x0,max_x0,swAAAA(b_max));
	max_y0 = mask_max(m_update_x,max_y0,max_y0,swBBBB(b_max));
	max_z0 = mask_max(m_update_x,max_z0,max_z0,swCCCC(b_max));
	// ------------------------------------------------------------------------
	min_x1 = mask_min(m_update_y,min_x1,min_x1,swAAAA(b_min));
	min_y1 = mask_min(m_update_y,min_y1,min_y1,swBBBB(b_min));
	min_z1 = mask_min(m_update_y,min_z1,min_z1,swCCCC(b_min));      
	// ------------------------------------------------------------------------      
	max_x1 = mask_max(m_update_y,max_x1,max_x1,swAAAA(b_max));
	max_y1 = mask_max(m_update_y,max_y1,max_y1,swBBBB(b_max));
	max_z1 = mask_max(m_update_y,max_z1,max_z1,swCCCC(b_max));
	// ------------------------------------------------------------------------
	min_x2 = mask_min(m_update_z,min_x2,min_x2,swAAAA(b_min));
	min_y2 = mask_min(m_update_z,min_y2,min_y2,swBBBB(b_min));
	min_z2 = mask_min(m_update_z,min_z2,min_z2,swCCCC(b_min));
	// ------------------------------------------------------------------------      
	max_x2 = mask_max(m_update_z,max_x2,max_x2,swAAAA(b_max));
	max_y2 = mask_max(m_update_z,max_y2,max_y2,swBBBB(b_max));
	max_z2 = mask_max(m_update_z,max_z2,max_z2,swCCCC(b_max));
	// ------------------------------------------------------------------------
	count0 = mask_add(m_update_x,count0,count0,mic_i::one());
	count1 = mask_add(m_update_y,count1,count1,mic_i::one());
	count2 = mask_add(m_update_z,count2,count2,mic_i::one());      
      }

    bin16.prefetchL2EX();

    bin16.min_x[0] = min_x0;
    bin16.min_y[0] = min_y0;
    bin16.min_z[0] = min_z0;
    bin16.max_x[0] = max_x0;
    bin16.max_y[0] = max_y0;
    bin16.max_z[0] = max_z0;

    bin16.min_x[1] = min_x1;
    bin16.min_y[1] = min_y1;
    bin16.min_z[1] = min_z1;
    bin16.max_x[1] = max_x1;
    bin16.max_y[1] = max_y1;
    bin16.max_z[1] = max_z1;

    bin16.min_x[2] = min_x2;
    bin16.min_y[2] = min_y2;
    bin16.min_z[2] = min_z2;
    bin16.max_x[2] = max_x2;
    bin16.max_y[2] = max_y2;
    bin16.max_z[2] = max_z2;

    bin16.count[0] = count0;
    bin16.count[1] = count1;
    bin16.count[2] = count2;

    bin16.thread_count[0] = count0; 
    bin16.thread_count[1] = count1; 
    bin16.thread_count[2] = count2;     
  }

  template<class Primitive, bool EXTEND_ATOMIC>
  __forceinline size_t partitionPrimitives(Primitive *__restrict__ const t_array,
					   const size_t size,
					   const BinPartitionMapping &mapping,
					   Centroid_Scene_AABB & local_left,
					   Centroid_Scene_AABB & local_right)
    {
      CentroidGeometryAABB leftReduction;
      CentroidGeometryAABB rightReduction;
      leftReduction.reset();
      rightReduction.reset();

      Primitive* l = t_array;
      Primitive* r = t_array + size - 1;

      while(1)
	{
	  /* *l < pivot */
	  while (likely(l <= r)) 
	    {
	      const mic2f bounds = l->getBounds();
	      const mic_f b_min  = bounds.x;
	      const mic_f b_max  = bounds.y;
	      prefetch<PFHINT_L1EX>(((char*)l)+4*64);

	      if (unlikely(mapping.ge_split(b_min,b_max))) break;

	      prefetch<PFHINT_L2EX>(((char*)l)+20*64);

	      leftReduction.extend(b_min,b_max);
	      ++l;
	    }
	  /* *r >= pivot) */
	  while (likely(l <= r))
	    {
	      const mic2f bounds = r->getBounds();
	      const mic_f b_min  = bounds.x;
	      const mic_f b_max  = bounds.y;
	      prefetch<PFHINT_L1EX>(((char*)r)-4*64);	  

	      if (unlikely(mapping.lt_split(b_min,b_max))) break;
	      prefetch<PFHINT_L2EX>(((char*)r)-20*64);	  

	      rightReduction.extend(b_min,b_max);
	      --r;
	    }

	  if (r<l) break;

	  rightReduction.extend(l->getBounds());
	  leftReduction.extend(r->getBounds());

	  xchg(*l,*r);
	  l++; r--;
	}
      
      if (!EXTEND_ATOMIC)
	{
	  local_left  = Centroid_Scene_AABB( leftReduction );
	  local_right = Centroid_Scene_AABB( rightReduction );
	}
      else
	{
	  Centroid_Scene_AABB update_left  = Centroid_Scene_AABB( leftReduction );
	  Centroid_Scene_AABB update_right = Centroid_Scene_AABB( rightReduction );
	  local_left.extend_atomic( update_left );
	  local_right.extend_atomic( update_right );
	}

      return l - t_array;              
    }


    /* shared structure for multi-threaded binning and partitioning */
    struct __aligned(64) SharedBinningPartitionData
    {
      __aligned(64) BuildRecord rec;
      __aligned(64) Centroid_Scene_AABB left;
      __aligned(64) Centroid_Scene_AABB right;
      __aligned(64) Split split;
    };

};
