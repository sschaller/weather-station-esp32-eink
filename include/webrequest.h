#ifndef WebRequest_h
#define WebRequest_h

#include <WiFiClientSecure.h>

class WebRequest {
  bool connected;
  
  public:
    WiFiClientSecure client;
    
    WebRequest(){};
    ~WebRequest();

    bool connect();
    void disconnect();

    bool requestWeather();
    bool updateTime();
};

#endif /* WebRequest_h */