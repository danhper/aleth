// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include <libdevcore/DBFactory.h>
#include <libdevcore/CommonIO.h>
#include <libdevcore/FileSystem.h>
#include <libdevcore/LoggingProgramOptions.h>
#include <libdevcore/SHA3.h>
#include <libethashseal/Ethash.h>
#include <libethashseal/GenesisInfo.h>
#include <libethcore/Common.h>
#include <libethcore/SealEngine.h>
#include <libethereum/Block.h>
#include <libethereum/ChainParams.h>
#include <libethereum/Executive.h>
#include <libethereum/LastBlockHashesFace.h>
#include <libevm/VMFactory.h>

#include <aleth/buildinfo.h>

#include <libevm-gas-exploiter/Benchmarker.h>
#include <libevm-gas-exploiter/ExecutionEnv.h>
#include <libevm-gas-exploiter/GeneticEngine.h>
#include <libevm-gas-exploiter/InstructionMetadata.h>
#include <libevmanalysis/StreamWrapper.h>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

#include <ctime>
#include <fstream>
#include <iostream>
#include <csignal>
#include <unistd.h>

using namespace std;
using namespace dev;
using namespace eth;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

namespace
{

enum class Mode
{
    Benchmark,
    Search,
    BenchmarkCache,
};

int64_t maxBlockGasLimit()
{
    static int64_t limit =
        ChainParams(genesisInfo(Network::MainNetwork)).maxGasLimit.convert_to<int64_t>();
    return limit;
}

void version()
{
    const auto* buildinfo = aleth_get_buildinfo();
    cout << "aleth-vm " << buildinfo->project_version << "\n";
    cout << "Build: " << buildinfo->system_name << "/" << buildinfo->build_type << "\n";
    exit(AlethErrors::Success);
}


void requireRoot(const std::string& message)
{
    if (getuid() != 0)
    {
        throw std::runtime_error(message);
    }
}


}

int main(int argc, char** argv)
{
    setDefaultOrCLocale();
    bool debug = false;
    unsigned int seed = 0;
    Address sender = Address(69);
    Address origin = Address(69);
    u256 value = 0;
    u256 gas = maxBlockGasLimit();
    u256 gasPrice = 0;
    Network networkName = Network::MainNetwork;
    uint64_t execCount = 1;
    bytes data;
    std::string metadataPath;
    Mode mode = Mode::Benchmark;
    std::string outputPath("-");
    std::string programsPath;
    uint64_t startIndex = 0;
    uint64_t endIndex = 0;
    bool warmup = true;
    uint16_t initialWarmupCount = 10;
    bool dropCache = false;
    bool alwaysDropCache = false;

    uint32_t populationSize = 1000;
    uint32_t initialProgramSize = 2000;
    uint32_t minimumProgramSize = 1000;
    uint32_t maximumProgramSize = 5000;
    uint32_t generationsCount = 1000;
    uint32_t mutationsCount = 5;
    bool cacheResults = false;
    double eliteRatio = 0.1;
    double tournamentSelectionProb = 0.4;
    double tournamentSelectionRatio = 0.2;
    GeneticEngine::Metric targetMetric = GeneticEngine::Metric::Mean;

    Ethash::init();
    NoProof::init();

    po::options_description transactionOptions("Transaction options", c_lineWidth);
    string const gasLimitDescription =
        "<n> Block gas limit (default: " + to_string(maxBlockGasLimit()) + ").";
    auto addTransactionOption = transactionOptions.add_options();
    addTransactionOption(
        "value", po::value<u256>(), "<n> Transaction should transfer the <n> wei (default: 0).");
    addTransactionOption("gas", po::value<u256>(),
        "<n> Transaction should be given <n> gas (default: block gas limit).");
    addTransactionOption("gas-limit", po::value<u256>(), gasLimitDescription.c_str());
    addTransactionOption(
        "gas-price", po::value<u256>(), "<n> Transaction's gas price' should be <n> (default: 0).");
    addTransactionOption("sender", po::value<Address>(),
        "<a> Transaction sender should be <a> (default: 0000...0069).");
    addTransactionOption("origin", po::value<Address>(),
        "<a> Transaction origin should be <a> (default: 0000...0069).");

    po::options_description networkOptions("Network options", c_lineWidth);
    networkOptions.add_options()("network", po::value<string>(),
        "Main|Ropsten|Homestead|Frontier|Byzantium|Constantinople|ConstantinopleFix\n");

    LoggingOptions loggingOptions;
    po::options_description loggingProgramOptions(
        createLoggingProgramOptions(c_lineWidth, loggingOptions));

    po::options_description generalOptions("General options", c_lineWidth);
    auto addGeneralOption = generalOptions.add_options();
    addGeneralOption("version,v", "Show the version and exit.");
    addGeneralOption("help,h", "Show this help message and exit.");
    addGeneralOption("debug", "Enables debug mode.");
    addGeneralOption("seed", po::value<unsigned int>(), "<s> Set random seed");
    addGeneralOption("mode", po::value<std::string>(), "<m> Mode to use (benchmark, search, benchmark-cache)");
    addGeneralOption("author", po::value<Address>(), "<a> Set author");
    addGeneralOption("difficulty", po::value<u256>(), "<n> Set difficulty");
    addGeneralOption("number", po::value<int64_t>(), "<n> Set number");
    addGeneralOption("timestamp", po::value<int64_t>(), "<n> Set timestamp");
    addGeneralOption("exec-count", po::value<uint64_t>(), "<n> Set execution count for input");
    addGeneralOption("initial-warmup-count", po::value<uint16_t>(), "Number of times to run warmup before the first measurement");
    addGeneralOption("no-warmup", "Do not warmup before each block execution");
    addGeneralOption("drop-cache", "Drop caches before starting benchmark");
    addGeneralOption("always-drop-cache", "Drop caches before each benchmark execution");
    addGeneralOption("metadata-path", po::value<std::string>(), "<p> Set the path for the metadata");
    addGeneralOption("output-path", po::value<std::string>(), "<p> Set the path to save results");
    addGeneralOption("programs-path", po::value<std::string>(), "<p> Set the path of the programs to benchmark");
    addGeneralOption("start-index", po::value<uint64_t>(), "<p> The first index (line number) of the programs to benchmark");
    addGeneralOption("end-index", po::value<uint64_t>(), "<p> The last index (line number) of the programs to benchmark");

    po::options_description gaOptions("Genetic algorithm options", c_lineWidth);
    auto addGaOption = gaOptions.add_options();
    addGaOption("population-size", po::value<uint32_t>(), "<n> Set population size");
    addGaOption("init-program-size", po::value<uint32_t>(), "<n> Set initial program size");
    addGaOption("min-program-size", po::value<uint32_t>(), "<n> Set minimum program size");
    addGaOption("max-program-size", po::value<uint32_t>(), "<n> Set maximum program size");
    addGaOption("cache-results", "Cache the results");
    addGaOption("generations-count", po::value<uint32_t>(), "<n> Set numbers of generation to run");
    addGaOption("mutations-count", po::value<uint32_t>(), "<n> Set numbers of mutations per sample");
    addGaOption("elite-ratio", po::value<double>(), "<x> Set the ratio of elite to keep across generations");
    addGaOption("tournament-selection-p", po::value<double>(), "<x> Set probability of picking first for tournament selection");
    addGaOption("tournament-selection-ratio", po::value<double>(), "<x> Set the ratio of samples to use for tournament selection");
    addGaOption("target-metric", po::value<std::string>(), "<m> Metric to optimize when performing the search ('mean' or 'median')");


    po::options_description dbOptions = db::databaseProgramOptions(c_lineWidth);

    po::options_description allowedOptions(
        "Usage aleth-vm-instr <options> (<file>|-)");
    allowedOptions.add(vmProgramOptions(c_lineWidth))
        .add(networkOptions)
        .add(loggingProgramOptions)
        .add(generalOptions)
        .add(dbOptions)
        .add(gaOptions)
        .add(transactionOptions);
    po::parsed_options parsed =
        po::command_line_parser(argc, argv).options(allowedOptions).allow_unregistered().run();
    vector<string> unrecognisedOptions =
        collect_unrecognized(parsed.options, po::include_positional);
    po::variables_map vm;
    po::store(parsed, vm);
    po::notify(vm);

    setupLogging(loggingOptions);

    if (vm.count("help"))
    {
        cout << allowedOptions;
        return AlethErrors::Success;
    }
    if (vm.count("version"))
    {
        version();
    }
    if (vm.count("debug"))
    {
        debug = true;
    }
    if (vm.count("seed"))
    {
        seed = vm["seed"].as<unsigned int>();
    }
    if (vm.count("mode"))
    {
        string modeName = vm["mode"].as<string>();
        if (modeName == "benchmark" )
            mode = Mode::Benchmark;
        else if (modeName == "search")
            mode = Mode::Search;
        else if (modeName == "benchmark-cache")
            mode = Mode::BenchmarkCache;
        else
        {
            std::cerr << "mode should be 'benchmark' or 'search', got '" << modeName << "'" << std::endl;
            return AlethErrors::UnknownArgument;
        }
    }

    if (vm.count("network"))
    {
        string network = vm["network"].as<string>();
        if (network == "ConstantinopleFix")
            networkName = Network::ConstantinopleFixTest;
        if (network == "Constantinople")
            networkName = Network::ConstantinopleTest;
        else if (network == "Byzantium")
            networkName = Network::ByzantiumTest;
        else if (network == "Frontier")
            networkName = Network::FrontierTest;
        else if (network == "Ropsten")
            networkName = Network::Ropsten;
        else if (network == "Homestead")
            networkName = Network::HomesteadTest;
        else if (network == "Main")
            networkName = Network::MainNetwork;
        else
        {
            cerr << "Unknown network type: " << network << "\n";
            return AlethErrors::UnknownNetworkType;
        }
    }

    ChainParams chainParams(genesisInfo(networkName), genesisStateRoot(networkName));
    WithExisting withExisting = WithExisting::Trust;
    InstructionsBenchmark instructionsBenchmark;
    auto analysisEnv = std::make_shared<AnalysisEnv>(std::cout, std::cout, instructionsBenchmark);
    auto dbPath = db::databasePath();

    auto blockchain = std::make_shared<BlockChain>(
        chainParams, dbPath, withExisting, [](unsigned, unsigned) {}, analysisEnv);

    auto stateDB = State::openDB(dbPath, blockchain->genesisHash(), withExisting);

    auto originalBlock = blockchain->genesisBlock(stateDB);
    originalBlock.sync(*blockchain);
    auto originalBlockHeader = originalBlock.info();
    originalBlockHeader.setGasLimit(maxBlockGasLimit());


    if (vm.count("sender"))
        sender = vm["sender"].as<Address>();
    if (vm.count("origin"))
        origin = vm["origin"].as<Address>();
    if (vm.count("gas"))
        gas = vm["gas"].as<u256>();
    if (vm.count("gas-price"))
        gasPrice = vm["gas-price"].as<u256>();
    if (vm.count("author"))
        originalBlockHeader.setAuthor(vm["author"].as<Address>());
    if (vm.count("difficulty"))
        originalBlockHeader.setDifficulty(vm["difficulty"].as<u256>());
    if (vm.count("number"))
        originalBlockHeader.setNumber(vm["difficulty"].as<int64_t>());
    if (vm.count("timestamp"))
        originalBlockHeader.setTimestamp(vm["timestamp"].as<int64_t>());
    if (vm.count("gas-limit"))
        originalBlockHeader.setGasLimit((vm["gas-limit"].as<u256>()).convert_to<int64_t>());
    if (vm.count("value"))
        value = vm["value"].as<u256>();
    if (vm.count("exec-count"))
        execCount = vm["exec-count"].as<uint64_t>();
    if (vm.count("start-index"))
        startIndex = vm["start-index"].as<uint64_t>();
    if (vm.count("end-index"))
        endIndex = vm["end-index"].as<uint64_t>();
    else
        endIndex = startIndex;
    if (vm.count("programs-path"))
        programsPath = vm["programs-path"].as<std::string>();
    if (vm.count("metadata-path"))
        metadataPath = vm["metadata-path"].as<std::string>();
    if (vm.count("output-path"))
        outputPath = vm["output-path"].as<std::string>();

    if (vm.count("initial-warmup-count"))
        initialWarmupCount = vm["initial-warmup-count"].as<uint16_t>();
    if (vm.count("no-warmup"))
        warmup = false;
    if (vm.count("drop-cache"))
    {
        requireRoot("must be root to drop caches");
        dropCache = true;
    }
    if (vm.count("always-drop-cache"))
    {
        requireRoot("must be root to drop caches");
        alwaysDropCache = true;
    }
    if (vm.count("population-size"))
        populationSize = vm["population-size"].as<uint32_t>();
    if (vm.count("init-program-size"))
        initialProgramSize = vm["init-program-size"].as<uint32_t>();
    if (vm.count("min-program-size"))
        minimumProgramSize = vm["min-program-size"].as<uint32_t>();
    if (vm.count("max-program-size"))
        maximumProgramSize = vm["max-program-size"].as<uint32_t>();
    if (vm.count("generations-count"))
        generationsCount = vm["generations-count"].as<uint32_t>();
    if (vm.count("mutations-count"))
        mutationsCount = vm["mutations-count"].as<uint32_t>();
    if (vm.count("cache-results"))
        cacheResults = true;
    if (vm.count("elite-ratio"))
        eliteRatio = vm["elite-ratio"].as<double>();
    if (vm.count("tournament-selection-p"))
        tournamentSelectionProb = vm["tournament-selection-p"].as<double>();
    if (vm.count("tournament-selection-ratio"))
        tournamentSelectionRatio = vm["tournament-selection-ratio"].as<double>();
    if (vm.count("target-metric"))
    {
        std::string targetMetricName = vm["target-metric"].as<std::string>();
        if (targetMetricName == "mean" )
            targetMetric = GeneticEngine::Metric::Mean;
        else if (targetMetricName == "median" )
            targetMetric = GeneticEngine::Metric::Median;
        else
        {
            std::cerr << "target-metric should be 'mean' or 'median', got '" << targetMetricName << "'" << std::endl;
            return AlethErrors::UnknownArgument;
        }
    }

    ExecutionEnv execEnv = {
        .block = originalBlock,
        .blockHeader = originalBlockHeader,
        .value = value,
        .gasPrice = gasPrice,
        .gas = gas,
        .sender = sender,
        .origin = origin,
        .chain = blockchain
    };

    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    if (mode == Mode::Benchmark)
    {
        if (programsPath.empty())
        {
            std::cerr << "'programs-path' should be set to benchmark" << std::endl;
            return AlethErrors::ArgumentProcessingFailure;
        }

        auto inWrapper = IStreamWrapper(programsPath);
        auto& inputStream = inWrapper.getStream();

        std::string line;
        for (uint64_t i = 0; i < startIndex && std::getline(inputStream, line); i++);

        std::vector<std::vector<bytes>> blocks;

        for (uint64_t i = startIndex; i <= endIndex; i++)
        {
            std::getline(inputStream, line);

            if (line.empty())
            {
                std::cerr << "line number " << i << " not found in " << programsPath << std::endl;
                return AlethErrors::ArgumentProcessingFailure;
            }

            Json::Value root;
            Json::Reader reader;
            if (!reader.parse(line, root))
            {
                std::cerr << "could not parse JSON" << std::endl;
                return AlethErrors::ArgumentProcessingFailure;
            }

            if (!(root.isMember("programs") && root["programs"].isArray()))
            {
                std::cerr << "JSON not formatted correctly, should contain 'programs' key" << std::endl;
                return AlethErrors::ArgumentProcessingFailure;
            }

            auto& jsonPrograms = root["programs"];

            std::vector<bytes> programs;
            for (Json::Value::ArrayIndex i = 0; i != jsonPrograms.size(); i++)
            {
                auto& jsonProgram = jsonPrograms[i];
                if (!(jsonProgram.isMember("program") && jsonProgram["program"].isMember("code")))
                {
                    std::cerr << "JSON not formatted correctly, should contain 'programs[n].program.code' key" << std::endl;
                    return AlethErrors::ArgumentProcessingFailure;
                }
                auto codeHex = jsonProgram["program"]["code"].asString();
                programs.push_back(fromHex(codeHex));
            }

            blocks.push_back(programs);
        }

        auto outputStreamWrapper = OStreamWrapper(outputPath, std::ios_base::trunc);
        BenchmarkConfig benchmarkConfig(execCount, debug, initialWarmupCount, warmup, dropCache, alwaysDropCache);
        auto blockNumber = originalBlockHeader.number();
        for (auto& programs : blocks)
        {
            auto results = benchmarkCodes(execEnv, programs, benchmarkConfig);
            auto jsonResults = results.toJson(true);
            jsonResults["blockNumber"] = blockNumber;
            writer->write(jsonResults, &outputStreamWrapper.getStream());
            outputStreamWrapper.getStream() << std::endl;
        }
    }
    else if (mode == Mode::Search)
    {
        std::map<Instruction, InstructionMetadata> instructionsMetadata;
        if (!metadataPath.empty())
        {
            instructionsMetadata = parseInstructionsFromFile(metadataPath);
        }

        GeneticEngine::TournamentSelectionConfig tournamentConfig(tournamentSelectionRatio, tournamentSelectionProb);
        BenchmarkConfig benchmarkConfig(execCount, debug, initialWarmupCount, warmup, dropCache);
        GeneticEngine::Config config{
            .populationSize = populationSize,
            .initialProgramSize = initialProgramSize,
            .minimumProgramSize = minimumProgramSize,
            .maximumProgramSize = maximumProgramSize,
            .generationsCount = generationsCount,
            .mutationsCount = mutationsCount,
            .eliteRatio = eliteRatio,
            .debug = debug,
            .cacheResults = cacheResults,
            .tournamentSelectionConfig = tournamentConfig,
            .execEnv = execEnv,
            .seed = seed,
            .targetMetric = targetMetric,
            .benchmarkConfig = benchmarkConfig,
        };

        auto programGenerator = std::make_shared<ProgramGenerator>(instructionsMetadata, seed);

        auto statStreamWrapper = OStreamWrapper(outputPath);

        GeneticEngine geneticEngine(config, programGenerator, statStreamWrapper.getStream());
        std::cerr << "Running for " << config.generationsCount << " generations" << std::endl;
        geneticEngine.run();
    }
    else if (mode == Mode::BenchmarkCache)
    {
        requireRoot("must be root to benchmark cache");

        auto programGenerator = std::make_shared<ProgramGenerator>(seed);
        auto outputStreamWrapper = OStreamWrapper(outputPath);
        auto& ostream = outputStreamWrapper.getStream();

        for (size_t i = 0; i < populationSize; i++)
        {
            if (i % 10 == 0)
            {
                std::cout << "progress: " << i << "/" << populationSize << std::endl;
            }

            auto withCacheConfig = BenchmarkConfig(execCount, debug, initialWarmupCount, warmup, dropCache, false);
            auto withoutCacheConfig = BenchmarkConfig(execCount, debug, initialWarmupCount, warmup, dropCache, true);
            auto program = programGenerator->generateInitialProgram(initialProgramSize);
            auto resultsWithCache = benchmarkCode(execEnv, program.toBytes(), withCacheConfig);
            auto resultsWithoutCache = benchmarkCode(execEnv, program.toBytes(), withoutCacheConfig);
            Json::Value result;
            result["code"] = program.toHex();
            result["with_cache"] = resultsWithCache.toJson();
            result["without_cache"] = resultsWithoutCache.toJson();
            writer->write(result, &ostream);
            ostream << std::endl;
        }
    }

    return AlethErrors::Success;
}
