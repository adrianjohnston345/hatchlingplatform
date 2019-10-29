#pragma once
// Copyright 2017 Adrian Johnston

#include "hatchling.h"

// ----------------------------------------------------------------------------
// hxFile: RAII wrapper for stream I/O.  A mixture of unformatted std::basic_fstream
// operations and formatted C-style text printing.

class hxFile {
public:
	enum openmode {
		in = 1u << 0, // open for binary reading
		out = 1u << 1, // open for binary writing
		fallible = 1u << 2,  // allow to fail.  similar to setting basic_ios::exceptions(0)
		binary = 0u, // always binary
		reserved_ = (uint16_t)~((1u << 3) - 1u) // reserved flags
	};

	hxFile();
	hxFile(uint16_t mode, const char* filename, ...) HX_ATTR_FORMAT(3, 4);
	~hxFile();

	bool open(uint16_t mode, const char* filename, ...) HX_ATTR_FORMAT(3, 4);
	void close(); // close stream and clear open flags

	// std::basic_fstream interface
	HX_INLINE bool is_open() const { return m_filePImpl != null; }
	HX_INLINE bool good() const { return m_good; } // Check for failure or EOF.
	HX_INLINE bool eof() const { return m_eof; }   // Check for EOF only.
	HX_INLINE void clear() {
		m_good = m_filePImpl != null;
		m_eof = false;
	}

	size_t read(void* bytes, size_t count);
	size_t write(const void* bytes, size_t count);

	// Reads an \n or EOF terminated character sequence.  Allowed to fail on
	// EOF without needing to be openmode::fallible.  
	template<size_t BufferSize>
	HX_INLINE bool getline(char(&buffer)[BufferSize]) { return getline(buffer, BufferSize); }

	bool getline(char* buffer, size_t bufferSize);

	// Formatted write.  Must be less than HX_MAX_LINE characters.
	bool print(const char* format, ...) HX_ATTR_FORMAT(2, 3); // gcc considers "this" argument 1.

	template<typename T>
	HX_INLINE size_t read1(T& t) { return read(&t, sizeof t); }

	template<typename T>
	HX_INLINE size_t write1(const T& t) { return write(&t, sizeof t); }

	// Unformatted binary stream read.  Expects little endian data.
	template<typename T>
	HX_INLINE hxFile& operator>>(T& t) {
		read(&t, sizeof t);
		return *this;
	}

	// Unformatted binary stream write.  Writes little endian data.
	template<typename T>
	HX_INLINE hxFile& operator<<(const T& t) {
		write(&t, sizeof t);
		return *this;
	}

private:
	hxFile(const hxFile&); // = delete
	void operator=(const hxFile&); // = delete
	template<typename T> HX_INLINE hxFile& operator<<(const T* t); // = delete
	template<typename T> HX_INLINE hxFile& operator>>(const T* t); // = delete

	bool openV(uint16_t mode, const char* format, va_list args);

	char* m_filePImpl;
	uint16_t m_openMode;
	bool m_good;
	bool m_eof;
};

