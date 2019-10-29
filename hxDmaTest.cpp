// Copyright 2017 Adrian Johnston
// Copyright 2017 Leap Motion

#include "hxDma.h"
#include "hxTest.h"

// ----------------------------------------------------------------------------------

class hxDmaTest :
	public testing::test
{
public:
	hxDmaTest() {
		setBuf();
	}

	~hxDmaTest() {
		hxDmaAwaitAll();
		hxDmaEndFrame();
		checkBuf(); // Don't trash test buffer
	}

	void setBuf(uint8_t* buf=null) {
		buf = buf ? buf : m_buf;
		for (uint32_t i = BufSize; i--;) {
			buf[i] = (uint8_t)i;
		}
	}

	void checkBuf(const uint8_t* buf = 0) const {
		buf = buf ? buf : m_buf;
		for (uint32_t i = BufSize; i--;) {
			ASSERT_EQ(buf[i], (uint8_t)i);
		}
	}

	static const uint32_t BufSize = 100;
	uint8_t m_buf[BufSize];
};

// ----------------------------------------------------------------------------------

TEST_F(hxDmaTest, Single) {
	uint8_t dst[BufSize];
	::memset(dst, 0x33, sizeof dst);
	hxDmaStart(dst, m_buf, BufSize);
	hxDmaAwaitAll();
	checkBuf(dst);
}

TEST_F(hxDmaTest, Multiple) {
	static const uint32_t OPS = 3u;
	uint8_t dst[OPS][BufSize];

	::memset(dst, 0x33, sizeof dst);
	for (uint32_t i = OPS; i--;) {
		hxDmaStart(dst[i], m_buf, BufSize);
	}
	hxDmaAwaitAll();
	for (uint32_t i = OPS; i--;) {
		checkBuf(dst[i]);
	}
}

TEST_F(hxDmaTest, Simultaneous) {
	static const uint32_t OPS = 3u;
	static const uint32_t REPS = 4u;
	uint8_t dst[OPS][BufSize];
	hxDmaSyncPoint sp[OPS];

	for (uint32_t j = REPS; j--;) {
		::memset(dst, 0x33, sizeof dst);
		for (uint32_t i = OPS; i--;) {
			hxDmaStart(dst[i], m_buf, BufSize);
			hxDmaAddSyncPoint(sp[i]);
		}
		for (uint32_t i = OPS; i--;) {
			hxDmaAwaitSyncPoint(sp[i]);
			checkBuf(dst[i]);
		}
	}
}



