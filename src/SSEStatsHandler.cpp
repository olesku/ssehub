#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <unistd.h>
#include <string>
#include <sstream>
#include <time.h>
#include "Common.h"
#include "SSEChannel.h"
#include "SSEServer.h"
#include "SSEClient.h"
#include "SSEStatsHandler.h"
#include "HTTPResponse.h"

/**
  Constructor.
*/
SSEStatsHandler::SSEStatsHandler() {
  _config = NULL;
  _server = NULL;

  invalid_http_req    = 0;
  oversized_http_req  = 0;
  invalid_events_rcv  = 0;
  router_read_errors  = 0;
}

/**
 Destructor.
*/
SSEStatsHandler::~SSEStatsHandler() {
}

/**
  Initializes the handler, needs to be called before Run().
  @param config Pointer to SSEConfig instance.
  @param server Pointer to SSEServer instance.
*/
void SSEStatsHandler::Init(SSEConfig* config, SSEServer* server) {
  _config = config;
  _server = server;
  _startTime = time(NULL);
}

void SSEStatsHandler::Update() {
  ulong totalClients     = 0;
  ulong totalEvents      = 0;
  ulong totalConnects    = 0;
  ulong totalDisconnects = 0;
  ulong totalErrors      = 0;
  uint  numChannels      = 0;

  boost::property_tree::ptree pt;
  boost::property_tree::ptree channels;

  BOOST_FOREACH(const SSEChannelPtr& chan, _server->GetChannelList()) {
    boost::property_tree::ptree pt_element;

    const SSEChannelStats& stat = chan->GetStats();

    totalClients     += stat.num_clients;
    totalEvents      += stat.num_broadcasted_events;
    totalConnects    += stat.num_connects;
    totalDisconnects += stat.num_disconnects;
    totalErrors      += stat.num_errors;
    numChannels++;

    pt_element.put("id", chan->GetId());
    pt_element.put("clients", stat.num_clients);
    pt_element.put("broadcasted_events", stat.num_broadcasted_events);
    pt_element.put("cached_events", stat.num_cached_events);
    pt_element.put("cache_size", stat.cache_size);
    pt_element.put("total_connects", stat.num_connects);
    pt_element.put("total_disconnects", stat.num_disconnects);
    pt_element.put("client_errors", stat.num_errors);

    channels.push_back(std::make_pair("", pt_element));
  }

  boost::posix_time::time_duration uptime;
  uptime = boost::posix_time::seconds((int)time(NULL) - _startTime);

  pt.put("global.uptime", boost::posix_time::to_simple_string(uptime));
  pt.put("global.clients", totalClients);
  pt.put("global.broadcasted_events", totalEvents);
  pt.put("global.channel_connects", totalConnects);
  pt.put("global.channel_disconnects", totalDisconnects);
  pt.put("global.channel_client_errors", totalErrors);
  pt.put("global.router_read_errors", router_read_errors);
  pt.put("global.invalid_http_req", invalid_http_req);
  pt.put("global.oversized_http_req", oversized_http_req);

  pt.put("global.channels", numChannels);

  if (numChannels > 0) {
    pt.add_child("channels", channels);
  }

  std::stringstream ss;
  write_json(ss, pt);

  _jsonData = ss.str();
}

/*
 Generate and return the statistics as JSON.
*/
const std::string& SSEStatsHandler::GetJSON() {
  Update();
  return _jsonData;
}

/**
 Send server statistics as JSON to client.
 @param client Pointer to SSEClient.
*/
void SSEStatsHandler::SendToClient(SSEClient* client) {
  HTTPResponse res;

  res.SetHeader("Content-Type", "application/json");
  res.SetHeader("Cache-Control", "no-cache");
  res.SetBody(GetJSON());

  client->Send(res.Get());
  client->FlushAndDestroy();
}
