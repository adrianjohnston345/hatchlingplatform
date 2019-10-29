#pragma once
// Copyright 2017-2019 Adrian Johnston
// Copyright 2017 Leap Motion

#include <hx/hatchling.h>

// ----------------------------------------------------------------------------
// Internally profiling and validating DMA API.  This code is intended to be
// customized for your platform.  If HX_PROFILE != 0 then hxDmaStart(),
// hxDmaAwaitSyncPoint() and hxDmaAwaitAll() calls generate descriptive labels.

// Used to marks a point in time so that preceding DMA operations can be waited for.
struct hxDmaSyncPoint {
#if HX_DEBUG_DMA
	// A synchronization point must be set with hxDmaAddSyncPoint() before use.
	hxDmaSyncPoint() : debugOnly(~(uint32_t)0u), pImpl(hxnull) { }
	uint32_t debugOnly;
#else
	hxDmaSyncPoint() : pImpl(hxnull) { }
#endif
	// TODO: Add a target specific handle.
	char* pImpl;
};

// Acquires and initializes resources required for DMA.  Called by hxInit().
void hxDmaInit();

#if (HX_RELEASE) < 3
// Releases resources required for DMA.  Called by hxShutdown();
void hxDmaShutDown();
#endif

// Waits for all DMA and invalidates all synchronization points.  Must be called
// intermittently when HX_DEBUG_DMA != 0 to recycle resources.
void hxDmaEndFrame();

// Marks a point in time so that preceding DMA operations can be waited for using
// syncPoint.
void hxDmaAddSyncPoint(hxDmaSyncPoint& syncPoint);

#if HX_PROFILE
// Initiates a DMA transfer from src to dst of bytes length.  labelStringLiteral,
// if non-null, is used in profiling and debug diagnostic messages.
#define hxDmaStart(dst, src, bytes, labelStringLiteral) \
	hxDmaStartLabeled(dst, src, bytes, labelStringLiteral)

// Waits until all DMA proceeding the corresponding call to hxDmaAddSyncPoint()
// is completed.  labelStringLiteral, if non-null, is used in a profiler record
// generated by this event and in profiling and debug diagnostic messages.
#define hxDmaAwaitSyncPoint(syncPoint, labelStringLiteral) \
	hxDmaAwaitSyncPointLabeled(syncPoint, labelStringLiteral)

// Waits until all DMA is completed.  labelStringLiteral, if non-null, is used
// in a profiler record generated by this event and in debug diagnostic messages.
#define hxDmaAwaitAll(labelStringLiteral) hxDmaAwaitAllLabeled(labelStringLiteral)

#else // !HX_PROFILE

#define hxDmaStart(dst, src, bytes, labelStringLiteral) hxDmaStartLabeled(dst, src, bytes, hxnull)
#define hxDmaAwaitSyncPoint(syncPoint, labelStringLiteral) hxDmaAwaitSyncPointLabeled(syncPoint, hxnull)
#define hxDmaAwaitAll(labelStringLiteral) hxDmaAwaitAllLabeled(hxnull)
#endif

// Use hxDmaStart to compile out labelStringLiteral when not profiling.
void hxDmaStartLabeled(void* dst, const void* src, size_t bytes, const char* labelStringLiteral);

// Use hxDmaAwaitSyncPoint to compile out labelStringLiteral when not profiling.
void hxDmaAwaitSyncPointLabeled(hxDmaSyncPoint& syncPoint, const char* labelStringLiteral);

// Use hxDmaAwaitAll to compile out labelStringLiteral when not profiling.
void hxDmaAwaitAllLabeled(const char* labelStringLiteral);