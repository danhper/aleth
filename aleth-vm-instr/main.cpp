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

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

#include <ctime>
#include <fstream>
#include <iostream>

using namespace std;
using namespace dev;
using namespace eth;
using namespace boost::accumulators;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

namespace
{

struct ExecutionEnv
{
    Block block;
    BlockHeader blockHeader;
    u256 value;
    u256 gasPrice;
    u256 gas;
    Address sender;
    Address origin;
    BlockChain& chain;
};

struct ExecutionStats
{
    u256 gasUsed;
    float executionTime;
    bytes output;
    TransactionException excepted;
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


ExecutionStats executeCode(bytes code, ExecutionEnv execEnv)
{
    auto block = execEnv.block;
    auto state = block.mutableState();
    auto blockHeader = execEnv.blockHeader;
    bytes data;

    Transaction t;
    Address contractDestination("1122334455667788991011121314151617181920");
    Account account(0, 0);
    account.setCode(bytes{code});
    std::unordered_map<Address, Account> map;
    map[contractDestination] = account;
    state.populateFrom(map);
    t = Transaction(execEnv.value, execEnv.gasPrice, execEnv.gas, contractDestination, data, 0);

    state.addBalance(execEnv.sender, execEnv.value);

    auto se = execEnv.chain.sealEngine();
    EnvInfo const envInfo(blockHeader, execEnv.chain.lastBlockHashes(), 0);
    Executive executive(state, envInfo, *se);
    ExecutionResult res;
    executive.setResultRecipient(res);
    t.forceSender(execEnv.sender);

    unordered_map<byte, pair<unsigned, bigint>> counts;

    executive.initialize(t);
    executive.call(contractDestination, execEnv.sender, execEnv.value, execEnv.gasPrice, &data, execEnv.gas);

    auto onOp = executive.traceInstructions();

    executive.go(onOp);
    executive.finalize();

    return ExecutionStats {
        .gasUsed = executive.gasUsed(),
        .executionTime = executive.executionTime(),
        .output = res.output,
        .excepted = res.excepted
    };
}

}

int main(int argc, char** argv)
{
    setDefaultOrCLocale();
    Address sender = Address(69);
    Address origin = Address(69);
    u256 value = 0;
    u256 gas = maxBlockGasLimit();
    u256 gasPrice = 0;
    Network networkName = Network::MainNetwork;
    uint64_t execCount = 1;
    bytes data;

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
    addTransactionOption("input", po::value<string>(), "<d> Transaction code should be <d>");
    addTransactionOption("code", po::value<string>(),
        "<d> Contract code <d>. Makes transaction a call to this contract");

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
    addGeneralOption("author", po::value<Address>(), "<a> Set author");
    addGeneralOption("difficulty", po::value<u256>(), "<n> Set difficulty");
    addGeneralOption("number", po::value<int64_t>(), "<n> Set number");
    addGeneralOption("timestamp", po::value<int64_t>(), "<n> Set timestamp");
    addGeneralOption("exec-count", po::value<uint64_t>(), "<n> Set execution count for input");

    po::options_description allowedOptions(
        "Usage aleth-vm-instr <options> (<file>|-)");
    allowedOptions.add(vmProgramOptions(c_lineWidth))
        .add(networkOptions)
        .add(loggingProgramOptions)
        .add(generalOptions)
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
    std::string snapshotPath;
    WithExisting withExisting = WithExisting::Trust;
    InstructionsBenchmark instructionsBenchmark(100);
    auto analysisEnv = std::make_shared<AnalysisEnv>(std::cout, std::cout, instructionsBenchmark);
    auto dbPath = db::databasePath();

    BlockChain blockchain(chainParams, dbPath, withExisting,
                          [](unsigned, unsigned) {}, analysisEnv);

    auto stateDB = State::openDB(dbPath, blockchain.genesisHash(), withExisting);

    auto originalBlock = blockchain.genesisBlock(stateDB);
    originalBlock.sync(blockchain);
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

    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    while (true)
    {
        std::string rawCode;
        std::cin >> rawCode;
        auto code = fromHex(rawCode);

        if (code.empty())
        {
            break;
        }

        accumulator_set<double, features<tag::sum, tag::mean, tag::variance>> timeMeasurements;
        u256 gasUsed = 0;
        bytes output;
        bool error = false;

        for (uint64_t i = 0; i < execCount; i++)
        {
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

            auto stats = executeCode(code, execEnv);
            auto exception = stats.excepted != TransactionException::None;
            if (exception)
            {
                std::cerr << "{\"error\": \"" << stats.excepted << "\"}" << std::endl;
                error = true;
                break;
            }

            if (output.empty())
            {
                output = stats.output;
            }
            else if (output != stats.output)
            {
                std::cerr << "{\"error\": \"obtained different output: '"
                          << output << "' != '" << stats.output << "'\"}" << std::endl;
                error = true;
                break;
            }

            if (gasUsed == 0)
            {
                gasUsed = stats.gasUsed;
            }
            else if (gasUsed != stats.gasUsed)
            {

                std::cerr << "{\"error\": \"obtained different gas used: '"
                          << gasUsed << "' != '" << stats.gasUsed << "'\"}" << std::endl;
                error = true;
                break;
            }

            timeMeasurements(stats.executionTime);
        }

        if (error)
        {
            continue;
        }

        Json::Value root;
        root["gas"] = gasUsed.convert_to<uint64_t>();
        double totalTime = sum(timeMeasurements);
        root["gas_per_second"] = gasUsed.convert_to<double>() / totalTime * execCount;
        root["time_mean"] = mean(timeMeasurements);
        root["time_stdev"] = sqrt(variance(timeMeasurements));
        writer->write(root, &std::cout);
        std::cout << std::endl;
    }

    return AlethErrors::Success;
}
