#include <libevmanalysis/BenchmarkResults.h>

#include <gtest/gtest.h>

using namespace std;
using namespace dev;
using namespace eth;

TEST(BenchmarkResults, addMeasurement)
{
    auto results = BenchmarkResults();
    results.addMeasurement(BenchmarkMeasurement(1, 1));
    EXPECT_EQ(results.count(), 1);
    results.addMeasurement(BenchmarkMeasurement(2, 1));
    EXPECT_EQ(results.count(), 2);
}

TEST(BenchmarkResults, mean)
{
    auto results = BenchmarkResults();
    for (auto i = size_t(1); i <= 7; i++)
    {
        results.addMeasurement(BenchmarkMeasurement(i, 1));
    }
    EXPECT_FLOAT_EQ(results.mean(), 4.0);
}

TEST(BenchmarkResults, stdev)
{
    auto results = BenchmarkResults();
    for (auto i = size_t(1); i <= 7; i++)
    {
        results.addMeasurement(BenchmarkMeasurement(i, 1));
    }
    EXPECT_FLOAT_EQ(results.stdev(), 2.0);
}

TEST(BenchmarkResults, addMeasurementWithGranularity)
{
    auto results = BenchmarkResults(2);
    results.addMeasurement(BenchmarkMeasurement(1, 1));
    EXPECT_EQ(results.count(), 0);
    results.addMeasurement(BenchmarkMeasurement(2, 1));
    EXPECT_EQ(results.count(), 1);
    results.addMeasurement(BenchmarkMeasurement(5, 1));
    EXPECT_EQ(results.count(), 1);
    results.addMeasurement(BenchmarkMeasurement(1, 1));
    EXPECT_EQ(results.count(), 2);
    EXPECT_FLOAT_EQ(results.mean(), 2.25);
}
