#include <libevmanalysis/StreamWrapper.h>
#include <stdio.h>

#include <gtest/gtest.h>

using namespace std;
using namespace dev;

#define FILENAME_S 128

std::ifstream::pos_type filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

void testFile(char* filename, int suffixLength)
{
    {
        auto fd = mkstemps(filename, suffixLength);
        ASSERT_NE(fd, 0);
        auto sw = OStreamWrapper(filename);
        for (size_t i = 0; i < 100; i++)
        {
            sw.getStream() << "hello" << i << std::endl;
        }
    }
    // NOTE: the file should be around 200 bytes when compressed
    EXPECT_GT(filesize(filename), 50);
    ASSERT_EQ(remove(filename), 0);
}

TEST(StreamWrapper, normalStream)
{
    char filename[] = "/tmp/aleth-XXXXXX.txt";
    testFile(filename, 4);
}

TEST(StreamWrapper, gzipStream)
{
    char filename[] = "/tmp/aleth-XXXXXX.txt.gz";
    testFile(filename, 7);
}
