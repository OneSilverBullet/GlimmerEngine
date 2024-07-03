#pragma once
#include "gpubuffer.h"

class ByteAddressBuffer : public GPUBuffer
{
protected:
    void CreateDerivedViews(void) override;
};