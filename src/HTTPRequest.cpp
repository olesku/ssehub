#include "Common.h"
#include <string.h>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "../lib/picohttpparser/picohttpparser.h"
#include "Common.h"
#include "HTTPRequest.h"

/**
  Constructor.
**/
HTTPRequest::HTTPRequest() {
  httpReq_bytesRead = 0;
  httpReq_bytesReadPrev = 0;
  httpReq_post_expected_size = 0;
  httpReq_post_bytesRead = 0;
  httpReq_isComplete = false;
  httpReq_isPost = false;
  post_data = "";
  error_message = "";
  b_success = false;
}

/**
  Destructor.
**/
HTTPRequest::~HTTPRequest() {}

/**
  Parse the request.
  @param data Raw http request data.
  @param len Length of data.
**/
HttpReqStatus HTTPRequest::Parse(const char *data, int len) {
  int pret;

  if (httpReq_isPost) {
    httpReq_post_bytesRead += len;

    if (httpReq_post_bytesRead > HTTP_POST_MAX) {
      DLOG(ERROR) << "HTTP_REQ_POST_TOO_LARGE " << "POST data is bigger than " << HTTP_POST_MAX;
      error_message = "HTTP_REQ_POST_TOO_LARGE: POST data is to large.";
      post_data.clear();
      return HTTP_REQ_POST_TOO_LARGE;
    }

    post_data.append(data);

    if (httpReq_post_bytesRead < httpReq_post_expected_size) {
      return HTTP_REQ_POST_INCOMPLETE;
    }

    return HTTP_REQ_POST_OK;
  }

  if (httpReq_isComplete) return HTTP_REQ_OK;
  httpReq_bytesReadPrev = httpReq_bytesRead;

  // Request is to large.
  if ((httpReq_bytesRead + len) > HTTPREQ_BUFSIZ) {
    error_message = "HTTP_REQ_TO_BIG: Request to large.";
    return HTTP_REQ_TO_BIG;
  }

  httpReq_bytesRead += len;
  httpReq_buf.append(data, len);

  phr_num_headers = sizeof(phr_headers) / sizeof(phr_headers[0]);

  pret = phr_parse_request(httpReq_buf.c_str(), httpReq_bytesRead, &phr_method, &phr_method_len, &phr_path,
      &phr_path_len, &phr_minor_version, phr_headers, &phr_num_headers, httpReq_bytesReadPrev);

  // Parse error.
  if (pret == -1) {
    DLOG(ERROR) << "HTTP_REQ_FAILED";
    error_message = "HTTP_REQ_FAILED: Parse failed.";
    return HTTP_REQ_FAILED;
  }

  // Request incomplete.
  if (pret == -2) {
    DLOG(INFO) << "HTTP_REQ_INCOMPLETE";
    return HTTP_REQ_INCOMPLETE;
  }

  if (phr_method_len > 0)
    method.insert(0, phr_method, phr_method_len);

  if (phr_path_len > 0) {
    string rawPath;
    rawPath.insert(0, phr_path, phr_path_len);

    size_t qsPos = rawPath.find_first_of('?', 0);
    if (qsPos != string::npos) {
      string qStr;
      qStr = rawPath.substr(qsPos+1, string::npos);
      path = rawPath.substr(0, qsPos);
      ParseQueryString(qStr);
    } else {
      path = rawPath.substr(0, rawPath.find_last_of(' ', 0));
    }

    size_t bp_epos = path.find_first_of("/", 1);
    basepath = (bp_epos != string::npos) ? path.substr(1, bp_epos-1) : path.substr(1);
    subpath = (bp_epos != string::npos) ? path.substr(bp_epos+1) : "";

    LOG(INFO) << "Basepath: " << basepath;
    LOG(INFO) << "Subpath: " << subpath;
  }

  for (int i = 0; i < (int)phr_num_headers; i++) {
    string name, value;
    name.insert(0, phr_headers[i].name, phr_headers[i].name_len);
    value.insert(0, phr_headers[i].value, phr_headers[i].value_len);
    boost::to_lower(name);
    headers[name] = value;
  }

  if (GetMethod().compare("POST") == 0) {
    if (GetHeader("Content-Length").empty()) {
      error_message = "HTTP_REQ_POST_INVALID_LENGTH: No Content-Length header set.";
      return HTTP_REQ_POST_INVALID_LENGTH;
    } else {
      try  {
        httpReq_post_expected_size = boost::lexical_cast<int>(GetHeader("Content-Length"));
      } catch(...) {
        error_message = "HTTP_REQ_POST_INVALID_LENGTH: Invalid format.";
        return HTTP_REQ_POST_INVALID_LENGTH;
      }
    }

    if (httpReq_post_expected_size < 1) {
      error_message = "HTTP_REQ_POST_INVAID_LENGTH: Cannot be zero.";
      return HTTP_REQ_POST_INVALID_LENGTH;
    }

    httpReq_isPost = true;

    // If we have post data in the initial request run Parse on it to take correct action.
    if (len > pret) {
      string tmp;
      tmp.insert(0, data, pret, len-pret);
      return Parse(tmp.c_str(), tmp.length());
    }

    return HTTP_REQ_POST_START;
 }

  httpReq_isComplete = true;
  httpReq_buf[httpReq_bytesRead] = '\0';
  DLOG(INFO) << "HTTP_REQ_OK"; 

  return HTTP_REQ_OK;
}

/**
  Get the HTTP request path.
**/
const string& HTTPRequest::GetPath() {
  return path;
}

/**
  Get the HTTP request path.
**/
const string& HTTPRequest::GetSubpath() {
  return subpath;
}

/**
  Get the HTTP request path.
**/
const string& HTTPRequest::GetBasepath() {
  return basepath;
}

/**
  Get the HTTP request method.
**/
const string& HTTPRequest::GetMethod() {
  return method;
}

/**
  Get a spesific header.
  @param header Header to get.
**/
const string HTTPRequest::GetHeader(string header) {
  boost::to_lower(header);

  if (headers.find(header) != headers.end()) {
    return headers[header];
  }

  return "";
}

const map<string, string>& HTTPRequest::GetHeaders() {
  return headers;
}

/**
  Returns true if the request was successful and false otherwise.
**/
bool HTTPRequest::Success() {
  return b_success;
}

/**
  Extracts query parameters from a string if they exist.
  @param buf The string to parse.
**/
size_t HTTPRequest::ParseQueryString(const std::string& buf) {
  size_t prevpos = 0, eqlpos = 0;

  while((eqlpos = buf.find("=", prevpos)) != string::npos) {
    string param, val;
    size_t len;

    len = buf.find("&", eqlpos);

    if (len != string::npos) len = (len - eqlpos);
    else len = (buf.size() - eqlpos);

    param = buf.substr(prevpos, (eqlpos - prevpos));

    eqlpos++;
    val = buf.substr(eqlpos, len-1);
    prevpos = eqlpos+len;

    if (!param.empty() && !val.empty()) {
      boost::to_lower(param);
      qsmap[param] = val;
    }
  }

  return qsmap.size();
}

/**
  Get a spesific query string parameter.
  @param param Parameter to get.
**/
const string HTTPRequest::GetQueryString(string param) {
  boost::to_lower(param);

  if (qsmap.find(param) != qsmap.end()) {
    return qsmap[param];
  }

  return "";
}

/**
  Returns number of query strings in the request.
**/
size_t HTTPRequest::NumQueryString() {
  return qsmap.size();
}

const string& HTTPRequest::GetPostData() {
  return post_data;
}

const string& HTTPRequest::GetErrorMessage() {
  return error_message;
}
