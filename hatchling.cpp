// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hatchling.h"
#include "hxMemoryManager.h"
#include "hxDma.h"
#include "hxProfiler.h"
#include "hxFile.h"
#include "hxConsole.h"

#include <stdio.h>

// ----------------------------------------------------------------------------

static const char* s_hxInitFile = ""; // For trapping code running before hxMain.
static uint32_t s_hxInitLine = 0;

// ----------------------------------------------------------------------------

extern "C"
void hxInitAt(const char* file, uint32_t line) {
	hxAssertRelease(!g_hxIsInit, "internal error");
	g_hxIsInit = 1;

	if (file) { s_hxInitFile = file; }
	s_hxInitLine = line;

	g_hxSettings.construct();

	hxLog("Hatchling Platform\n");
	hxConsolePrint("HX_RELEASE %d.  HX_PROFILE %d.  Last full build: " __DATE__ " " __TIME__ "\n", HX_RELEASE, HX_PROFILE);

	hxMemoryManagerInit();
	hxDmaInit();

#ifdef __cpp_rtti
	hxWarn("RTTI is enabled"); // RTTI is not advised.
#endif
#ifdef __cpp_exceptions
	hxWarn("Exceptions are enabled"); // Exceptions are not used.
#endif
#if !HX_HAS_CPP11_THREADS
	hxWarn("Single threaded only"); // Profiler and TaskQueue need custom threading.
#endif
}

#if (HX_RELEASE) < 3
// Wrapped to ensure correct construction order.
static hxFile& hxLogFile() {
	static hxFile f(hxFile::out | hxFile::fallible, "%s", g_hxSettings.logFile);
	return f;
}

extern "C"
void hxShutdown() {
	hxAssertRelease(g_hxIsInit, "hxShutdown unexpected\n");
	hxLogRelease("hxShutdown...\n");

	hxProfilerShutdown();
	hxDmaShutDown();

	g_hxSettings.isShuttingDown = true;
	hxConsoleDeregisterAll(); // Free console allocations.
	hxMemoryManagerShutDown();
#if (HX_MEM_DIAGNOSTIC_LEVEL) >= 1
	g_hxSettings.disableMemoryManager = true;
#endif
}

extern "C"
void hxExit(const char* format, ...) {
	char buf[HX_MAX_LINE];
	int sz = -1;
	if (format != null) {
		va_list args;
		va_start(args, format);
		sz = ::vsnprintf(buf, HX_MAX_LINE, format, args);
		va_end(args);
	}

#if HX_HAS_C_FILE
	if (sz < 0) {
		::fwrite("exit format error\n", 1, sizeof "exit format error\n" - 1, stdout);
	}
	else {
		::fwrite(buf, 1, (size_t)((sz < HX_MAX_LINE) ? sz : HX_MAX_LINE), stdout);
	}
	::fflush(stdout); 
#endif

	hxFile& f = hxLogFile();
	if (f.is_open()) {
		f.print("%s", (sz < 0) ? "exit format error\n" : buf);
		f.close();
	}

	// Stop here before the callstack gets lost inside _Exit.  This is not for
	// normal termination on an embedded target.
	HX_DEBUG_BREAK;
	::_Exit(EXIT_FAILURE);
}

extern "C"
void hxAssertHandler(const char* file, uint32_t line) {
	hxInit();
#if (HX_RELEASE) < 1
	if (g_hxSettings.assertsToBeSkipped-- > 0) {
		return;
	}
#endif
	if (file != null) {
		const char* f = hxBasename(file);
		hxExit("ASSERT_FAIL: %s(%u)\n", f, (unsigned int)line);
	}
	else
	{
		hxExit("ASSERT_FAIL\n");
	}
}

extern "C"
void hxLogHandlerV(enum hxLogLevel level, const char* format, va_list args) {
	hxInit();
	if (level < g_hxSettings.logLevelConsole && level < g_hxSettings.logLevelFile) {
		return;
	}

	char buf[HX_MAX_LINE+1];
	int sz = format ? ::vsnprintf(buf, HX_MAX_LINE, format, args) : 0;
	hxAssertMsg(sz >= 0, "format error: %s", format ? format : "<null>");
	if (sz <= 0) {
		return;
	}
	sz = hxMin(sz, HX_MAX_LINE);
	if (level == hxLogLevel_Warning || level == hxLogLevel_Assert) {
		buf[sz++] = '\n';
	}

#if HX_HAS_C_FILE
#define HX_FWRITE_STR(x) ::fwrite(x, 1, sizeof x - 1, stdout);
	if (level >= g_hxSettings.logLevelConsole) {
		if (level == hxLogLevel_Warning) {
			HX_FWRITE_STR("WARNING: ");
		}
		else if (level == hxLogLevel_Assert) {
			HX_FWRITE_STR("ASSERT_FAIL: ");
		}
		::fwrite(buf, 1, sz, stdout);
	}
#undef HX_FWRITE_STR
#endif

	if (level >= g_hxSettings.logLevelFile) {
		hxFile& f = hxLogFile();
		if (f.is_open()) {
			if (level == hxLogLevel_Warning) {
				f.print("WARNING: ");
			}
			else if (level == hxLogLevel_Assert) {
				f.print("ASSERT_FAIL: ");
			}

			f.write(buf, sz);
		}
	}
}


#else // #if (HX_RELEASE) >= 3

extern "C"
void hxLogHandlerV(enum hxLogLevel level, const char* format, va_list args) {
	::vfprintf(stdout, format, args);
}

#endif

extern "C"
void hxLogHandler(enum hxLogLevel level, const char* format, ...) {
	va_list args;
	va_start(args, format);
	hxLogHandlerV(level, format, args);
	va_end(args);
}