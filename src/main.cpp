#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include <boost/program_options.hpp>
#include "Common.h"
#include "SSEConfig.h"
#include "SSEServer.h"
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

int main(int argc, char **argv) {
  struct sigaction sa;
  int i;
  int n = 3;
  pid_t pids[5];

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

  sa.sa_handler = shutdown;
  sa.sa_flags   = 0;

  sigemptyset(&(sa.sa_mask));
  sigaction(SIGINT, &sa, NULL);

  for (i=0; i<n; ++i) {
    pids[i] = fork();

    if (pids[i] < 0) {
      LOG(ERROR) << "Could not fork fork() worker " << i;
      abort();
    } else if (pids[i] == 0) {
      SSEConfig conf;
      conf.load(conf_path.c_str());

      SSEServer server(&conf);
      server.Run();
      exit(0);
    }

    LOG(INFO) << "Started worker with PID: " << pids[i];
  }

  int status;
  for(i = 0; i < n; ++i) {
    waitpid(pids[i], &status, 0);
    LOG(INFO) << "Worker with pid " << pids[i] << " exited.";
  }


  return 0;
}
