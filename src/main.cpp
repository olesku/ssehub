#include <iostream>
#include <signal.h>
#include <boost/program_options.hpp>
#include "Common.h"
#include "SSEConfig.h"
#include "SSEServer.h"
#include <boost/thread/thread.hpp>
#define DEFAULT_CONFIG_FILE "./conf/config.json"

using namespace std;
namespace po = boost::program_options;

int stop = 0;
boost::thread_group serverThreads;

void shutdown(int sigid) {
  LOG(INFO) << "Exiting.";
  stop = 1;
  serverThreads.interrupt_all();
}

po::variables_map parse_options(po::options_description desc, int argc, char **argv) {
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);

  po::notify(vm);

  return vm;
}

void ServerThread(SSEConfig &conf) {
  SSEServer server(&conf);
  server.Run();
}

int main(int argc, char **argv) {
  struct sigaction sa;
  int i;

  FLAGS_logtostderr = 1;
  google::InitGoogleLogging(argv[0]);

  po::options_description desc("Options");
  desc.add_options()
    ("help", "produce help message")
    ("config", po::value<std::string>()->default_value(DEFAULT_CONFIG_FILE), "specify location of config file");

  po::variables_map vm = parse_options(desc, argc, argv);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  std::string conf_path = vm["config"].as<std::string>();
  SSEConfig conf;
  conf.load(conf_path.c_str());

  sa.sa_handler = shutdown;
  sa.sa_flags   = 0;

  sigemptyset(&(sa.sa_mask));
  sigaction(SIGINT, &sa, NULL);

  for (i=0; i<4; i++) {
    LOG(INFO) << "Starting worker " << (i+1);
    serverThreads.create_thread(boost::bind(&ServerThread, conf));
    cout << endl;
  }

  serverThreads.join_all();

  return 0;
}
