#include <libevmanalysis/BenchmarkResults.h>

#include <gtest/gtest.h>

using namespace std;
using namespace dev;
using namespace eth;

TEST(BenchmarkResults, addMeasurement)
{
    auto results = BenchmarkResults();
    results.addMeasurement(1);
    EXPECT_EQ(results.count(), 1);
    results.addMeasurement(2);
    EXPECT_EQ(results.count(), 2);
}

TEST(BenchmarkResults, sum)
{
    auto results = BenchmarkResults();
    results.addMeasurement(1);
    EXPECT_EQ(results.sum(), 1);
    results.addMeasurement(2);
    EXPECT_EQ(results.sum(), 3);
}

TEST(BenchmarkResults, squaredSum)
{
    auto results = BenchmarkResults();
    results.addMeasurement(1);
    EXPECT_EQ(results.squaredSum(), 1);
    results.addMeasurement(2);
    EXPECT_EQ(results.squaredSum(), 5);
}

TEST(BenchmarkResults, mean)
{
    auto results = BenchmarkResults();
    for (auto i = size_t(1); i <= 7; i++) {
        results.addMeasurement(i);
    }
    EXPECT_FLOAT_EQ(results.mean(), 4.0);
}

TEST(BenchmarkResults, stdev)
{
    auto results = BenchmarkResults();
    for (auto i = size_t(1); i <= 7; i++) {
        results.addMeasurement(i);
    }
    EXPECT_FLOAT_EQ(results.stdev(), 2.0);
}

TEST(BenchmarkResults, addMeasurementWithGranularity)
{
    auto results = BenchmarkResults(2);
    results.addMeasurement(1);
    EXPECT_EQ(results.count(), 0);
    results.addMeasurement(2);
    EXPECT_EQ(results.count(), 1);
    results.addMeasurement(5);
    EXPECT_EQ(results.count(), 1);
    results.addMeasurement(1);
    EXPECT_EQ(results.count(), 2);
    EXPECT_FLOAT_EQ(results.mean(), 2.25);
}
