#pragma once

#include "util/types.h"

class SideChainWorker {
  public:
    SideChainWorker() { }
    virtual ~SideChainWorker() = default;
    virtual void process(const CSAMPLE* pBuffer, const int iBufferSize) = 0;
    virtual void shutdown() = 0;
    // Called on the worker thread when the input FIFO lost samples.
    virtual void onBufferOverflow() {}
};
