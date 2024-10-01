/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */

#pragma once

#include <CubismFramework.hpp>
#include <ICubismAllocator.hpp>

class CCubismAllocator : public Csm::ICubismAllocator
{

    void* Allocate(const Csm::csmSizeType size);
    void Deallocate(void* memory);
    void* AllocateAligned(const Csm::csmSizeType size, const Csm::csmUint32 alignment);
    void DeallocateAligned(void* alignedMemory);
};
