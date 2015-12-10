#ifndef SSEEVENT_H
#define SSEEVENT_H

#include <string>
#include <sstream>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <glog/logging.h>

using namespace std;

class SSEEvent {
  public:
    SSEEvent(const string& jsonData);
    ~SSEEvent();
    bool  compile();
    const string get();
    const string getpath();
    const string getbasepath();
    const string getsubpath();
    const string getid();
    void  setpath(const string path);

  private:
    stringstream _json_ss;
    string _event;
    string _path;
    string _basepath;
    string _subpath;
    vector<string> _data;
    string _id;
    int _retry;
};

#endif
