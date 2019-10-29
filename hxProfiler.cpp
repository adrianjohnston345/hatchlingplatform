// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hxProfiler.h"
#include "hxArray.h"
#include "hxFile.h"
#include "hxConsole.h"

#if HX_PROFILE

hxProfiler g_hxProfiler;

// ----------------------------------------------------------------------------------
// Console commands

static void hxProfile() { g_hxProfiler.start(); }
hxConsoleCommandNamed(hxProfile, profileStart);

static void hxProfileLog() { g_hxProfiler.log(); }
hxConsoleCommandNamed(hxProfileLog, profileLog);

static void hxProfileToChrome(const char* filename) { g_hxProfiler.writeToChromeTracing(filename); }
hxConsoleCommandNamed(hxProfileToChrome, profileToChrome);

// ----------------------------------------------------------------------------------
#if HX_HAS_CPP11_THREADS
#include <mutex>

// Use the address of a thread local variable as a unique thread id.
static thread_local uint8_t s_hxProfilerThreadIdAddress = 0;
static std::mutex s_hxProfilerMutex;

hxProfilerScopeInternal::~hxProfilerScopeInternal() {
	uint32_t t1 = hxProfilerScopeInternal::sampleCycles();
	uint32_t delta = (t1 - m_t0);
	hxProfiler& data = g_hxProfiler;
	if (data.m_isEnabled && delta >= m_minCycles) {
		std::unique_lock<std::mutex> lk(s_hxProfilerMutex);
		if (!data.m_records.full()) {
			::new (data.m_records.emplace_back_unconstructed()) hxProfiler::Record(m_t0, t1, m_label, (uint32_t)(ptrdiff_t)&s_hxProfilerThreadIdAddress);
		}
	}
}
#endif

// ----------------------------------------------------------------------------------
#if HX_HAS_CPP11_TIME
#include <chrono>

using c_hxProfilerPeriod = std::chrono::high_resolution_clock::period;

const float g_hxProfilerMillisecondsPerCycle = ((float)c_hxProfilerPeriod::num * 1.0e+3f) / (float)c_hxProfilerPeriod::den;
const uint32_t g_hxProfilerDefaultSamplingCutoff = (c_hxProfilerPeriod::den / (c_hxProfilerPeriod::num * 1000000));

static std::chrono::high_resolution_clock::time_point g_hxStart;
uint32_t hxProfilerScopeInternal::sampleCycles() {
	return (uint32_t)(std::chrono::high_resolution_clock::now() - g_hxStart).count();
}
#else
// TODO: Use target settings.
const float g_hxProfilerMillisecondsPerCycle = 1.0e-6f; // Also 1.e+6 cycles/ms, a 1 GHz chip.
const uint32_t g_hxProfilerDefaultSamplingCutoff = 1000; // ~1 microsecond.
#endif // !HX_HAS_CPP11_TIME

// ----------------------------------------------------------------------------------

void hxProfiler::init() {
	if (m_isEnabled) {
		hxWarn("Profiler already enabled");
		return;
	}

	m_isEnabled = true;
	m_records.get_allocator().reserveStorageExt(HX_PROFILER_MAX_RECORDS, hxMemoryManagerId_Heap);
	m_records.reserve(HX_PROFILER_MAX_RECORDS);

#if HX_HAS_CPP11_TIME
	g_hxStart = std::chrono::high_resolution_clock::now();
#endif
	hxLogRelease("hxProfilerInit... %u cycles\n", (unsigned int)hxProfilerScopeInternal::sampleCycles()); // Logging may easily be off at this point.
}

void hxProfiler::shutdown() {
	if (m_isEnabled) {
		hxLogRelease("hxProfilerShutdown... %u cycles\n", (unsigned int)hxProfilerScopeInternal::sampleCycles());
	}
	m_isEnabled = false;

	// Force free-ing buffer.  Otherwise the destructor will crash the memory manager.
	m_records.~hxArray();
	::new(&m_records) hxArray<Record>();
}

void hxProfiler::start() {
	if (!m_isEnabled) {
		init();
	}
	else {
		m_records.clear();
	}
}

void hxProfiler::log() {
	if (m_records.empty()) {
		hxLogRelease("hxProfiler no samples\n");
	}

	for (uint32_t i = 0; i < m_records.size(); ++i) {
		const hxProfiler::Record& rec = m_records[i];

		uint32_t delta = rec.m_end - rec.m_begin;
		hxLogRelease("hxProfiler %s: thread %x cycles %u %fms\n", hxBasename(rec.m_label), (unsigned int)rec.m_threadId, (unsigned int)delta, (float)delta * g_hxProfilerMillisecondsPerCycle);
	}

	m_records.clear();
}

void hxProfiler::writeToChromeTracing(const char* filename) {
	hxFile f(hxFile::out, "%s", filename);

	if (m_records.empty()) {
		f.print("[]\n");
		hxLogRelease("Trace has no samples: %s...\n", filename);
		return;
	}

	f.print("[\n");
	// Converting absolute values works better with integer precision.
	uint32_t cyclesPerMicrosecond = (uint32_t)(1.0e-3f / g_hxProfilerMillisecondsPerCycle);
	for (uint32_t i = 0; i < m_records.size(); ++i) {
		const hxProfiler::Record& rec = m_records[i];
		if (i != 0) {
			f.print(",\n");
		}
		const char* bn = hxBasename(rec.m_label);
		f.print("{\"name\":\"%s\",\"cat\":\"PERF\",\"ph\":\"B\",\"pid\":0,\"tid\":%u,\"ts\":%u},\n",
			bn, (unsigned int)rec.m_threadId, (unsigned int)(rec.m_begin / cyclesPerMicrosecond));
		f.print("{\"name\":\"%s\",\"cat\":\"PERF\",\"ph\":\"E\",\"pid\":0,\"tid\":%u,\"ts\":%u}",
			bn, (unsigned int)rec.m_threadId, (unsigned int)(rec.m_end / cyclesPerMicrosecond));
	}
	f.print("\n]\n");

	m_records.clear();

	hxLogRelease("Wrote trace to: %s...\n", filename);
}

#endif // HX_PROFILE