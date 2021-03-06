#include <pgmspace.h>

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    String reply_str = processor("STATE");
    client->printf("%s", reply_str.c_str());
    client->ping();
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
  }
  else if (type == WS_EVT_ERROR)
  {
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
  }
  else if (type == WS_EVT_PONG)
  {
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len)
    {
      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < info->len; i++)
        {
          msg += (char)data[i];
        }
        Serial.println(msg);
        Serial.println(processRequest(msg) ? "Success" : "Failed");
      }
    }
  }
}

void handleNotFound(AsyncWebServerRequest *request)
{
  String filename = request->url();
  String ContentType = "text/plain";

  if (filename.endsWith(".htm"))
    ContentType = "text/html";
  else if (filename.endsWith(".html"))
    ContentType = "text/html";
  else if (filename.endsWith(".css"))
    ContentType = "text/css";
  else if (filename.endsWith(".js"))
    ContentType = "application/javascript";
  else if (filename.endsWith(".png"))
    ContentType = "image/png";
  else if (filename.endsWith(".gif"))
    ContentType = "image/gif";
  else if (filename.endsWith(".jpg"))
    ContentType = "image/jpeg";
  else if (filename.endsWith(".ico"))
    ContentType = "image/x-icon";
  else if (filename.endsWith(".xml"))
    ContentType = "text/xml";
  else if (filename.endsWith(".pdf"))
    ContentType = "application/x-pdf";
  else if (filename.endsWith(".zip"))
    ContentType = "application/x-zip";
  else if (filename.endsWith("ico.gz"))
    ContentType = "image/x-icon";
  else if (filename.endsWith(".gz"))
    ContentType = "application/x-gzip";

  if (SPIFFS.exists(filename + ".gz") || SPIFFS.exists(filename))
  {
    if (SPIFFS.exists(filename + ".gz"))
      filename += ".gz";
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, filename, ContentType);
    if (filename.endsWith(".gz"))
      response->addHeader("Content-Encoding", "gzip");
    request->send(response);
    return;
  }

  Serial.print("NOT_FOUND: ");
  if (request->method() == HTTP_GET)
    Serial.print("GET");
  else if (request->method() == HTTP_POST)
    Serial.print("POST");
  else if (request->method() == HTTP_DELETE)
    Serial.print("DELETE");
  else if (request->method() == HTTP_PUT)
    Serial.print("PUT");
  else if (request->method() == HTTP_PATCH)
    Serial.print("PATCH");
  else if (request->method() == HTTP_HEAD)
   Serial.print("HEAD");
  else if (request->method() == HTTP_OPTIONS)
    Serial.print("OPTIONS");
  else
    Serial.print("UNKNOWN");

  Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

  if (request->contentLength())
  {
    Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
    Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
  }

  int headers = request->headers();
  int i;
  for (i = 0; i < headers; i++)
  {
    AsyncWebHeader *h = request->getHeader(i);
    Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
  }

  int params = request->params();
  for (i = 0; i < params; i++)
  {
    AsyncWebParameter *p = request->getParam(i);
    if (p->isFile())
    {
      Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
    }
    else if (p->isPost())
    {
      Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
    }
    else
    {
      Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
    }
  }

  request->send(404);
}

void handleRoot(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/index.htm", String(), false, processor);
  request->send(response);
}

void handleUpload(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", upload_min_htm_gz, upload_min_htm_gz_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}

void processUploadReply(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", "<META http-equiv='refresh' content='3;URL=/'><body align=center><H4>Upload Success</H4></body>");
  response->addHeader("Connection", "close");
  request->send(response);
}

File fsUploadFile;

void processUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    Serial.printf("UploadStart: %s\n", filename.c_str());
    if (!filename.startsWith("/"))
      filename = "/" + filename;
    if (SPIFFS.exists(filename))
      SPIFFS.remove(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
  }
  for (size_t i = 0; i < len; i++)
    fsUploadFile.write(data[i]);
  if (final)
  {
    fsUploadFile.close();
    Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index + len);
  }
}

void handleUpdate(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", update_min_htm_gz, update_min_htm_gz_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}

void processUpdate(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!filename.endsWith(".bin"))
  {
    return;
  }
  if (!index)
  {
    Serial.printf("Update Start: %s\n", filename.c_str());
#ifdef ESP32
    uint32_t maxSketchSpace = len; // for ESP32 you just supply the length of file
#elif defined(ESP8266)
    Update.runAsync(true); // There is no async for ESP32
    uint32_t maxSketchSpace = ((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000);
#endif
    if (!Update.begin(maxSketchSpace))
    {
      Update.printError(Serial);
    }
  }
  if (!Update.hasError())
  {
    if (Update.write(data, len) != len)
    {
      Update.printError(Serial);
    }
  }
  if (final)
  {
    if (Update.end(true))
      Serial.printf("Update Success: %uB\n", index + len);
    else
    {
#ifdef SERIALDEBUG
      Update.printError(Serial);
#endif
    }
  }
}

void processUpdateReply(AsyncWebServerRequest *request)
{
  shouldReboot = !Update.hasError();
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", "<META http-equiv='refresh' content='3;URL=/'><body align=center>" + String(shouldReboot ? "OK" : "FAIL") + "</body>");
  response->addHeader("Connection", "close");
  request->send(response);
}