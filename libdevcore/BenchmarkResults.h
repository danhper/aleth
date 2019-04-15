#pragma once

struct BenchmarkResults {
    BenchmarkResults(): mean(0.0), variance(0.0) {}
    BenchmarkResults(double _mean, double _variance): mean(_mean), variance(_variance) {}
    double mean;
    double variance;
};
