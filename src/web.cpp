#include "main.h"
#include "web.h"
#include <FS.h>

Task WEB_task(0, TASK_FOREVER, &web_task_callback);

String unsupportedFiles = String();
File uploadFile;
static const char TEXT_PLAIN[] PROGMEM = "text/plain";
static const char FS_INIT_ERROR[] PROGMEM = "FS INIT ERROR";
static const char FILE_NOT_FOUND[] PROGMEM = "FileNotFound";

void replyOK() {
  Wserver.send(200, FPSTR(TEXT_PLAIN), "");
}

void replyOKWithMsg(String msg) {
  Wserver.send(200, FPSTR(TEXT_PLAIN), msg);
}

void replyNotFound(String msg) {
  Wserver.send(404, FPSTR(TEXT_PLAIN), msg);
}

void replyBadRequest(String msg) {
  print_DEBUG(msg);
  Wserver.send(400, FPSTR(TEXT_PLAIN), msg + "\r\n");
}

void replyServerError(String msg) {
  print_DEBUG(msg);
  Wserver.send(500, FPSTR(TEXT_PLAIN), msg + "\r\n");
}

void handleStatus() {
  DBG_OUTPUT_PORT.println("handleStatus");
  FSInfo fs_info;
  String json;
  json.reserve(128);

  json = "{\"type\":\"";
  json += fsName;
  json += "\", \"isOk\":";
  fileSystem->info(fs_info);
  json += F("\"true\", \"totalBytes\":\"");
  json += fs_info.totalBytes;
  json += F("\", \"usedBytes\":\"");
  json += fs_info.usedBytes;
  json += "\"";
  json += F(",\"unsupportedFiles\":\"");
  json += unsupportedFiles;
  json += "\"}";
  Wserver.send(200, "application/json", json);
}

void handleFileList() {

  if (!Wserver.hasArg("dir")) {
    return replyBadRequest(F("DIR ARG MISSING"));
  }

  String path = Wserver.arg("dir");
  if (path != "/" && !fileSystem->exists(path)) {
    return replyBadRequest("BAD PATH");
  }
  print_DEBUG(String("handleFileList: ") + path);
  Dir dir = fileSystem->openDir(path);
  path.clear();

  // use HTTP/1.1 Chunked response to avoid building a huge temporary string
  if (!Wserver.chunkedResponseModeStart(200, "text/json")) {
    Wserver.send(505, F("text/html"), F("HTTP1.1 required"));
    return;
  }

  // use the same string for every line
  String output;
  output.reserve(64);
  while (dir.next()) {
    if (output.length()) {
      // send string from previous iteration
      // as an HTTP chunk
      Wserver.sendContent(output);
      output = ',';
    } else {
      output = '[';
    }

    output += "{\"type\":\"";
    if (dir.isDirectory()) {
      output += "dir";
    } else {
      output += F("file\",\"size\":\"");
      output += dir.fileSize();
    }

    output += F("\",\"name\":\"");
    // Always return names without leading "/"
    if (dir.fileName()[0] == '/') {
      output += &(dir.fileName()[1]);
    } else {
      output += dir.fileName();
    }

    output += "\"}";
  }

  // send last string
  output += "]";
  Wserver.sendContent(output);
  Wserver.chunkedResponseFinalize();
}

bool handleFileRead(String path) {
    print_DEBUG(String("handleFileRead: ") + path);
    if (path.endsWith("/")) {
        path += "index.htm";
    }

    String contentType;
    if (Wserver.hasArg("download")) {
        contentType = F("application/octet-stream");
    } else {
        contentType = mime::getContentType(path);
    }

    if (!fileSystem->exists(path)) {
        // File not found, try gzip version
        path = path + ".gz";
    }
    if (fileSystem->exists(path)) {
        File file = fileSystem->open(path, "r");
        if (Wserver.streamFile(file, contentType) != file.size()) {
          print_DEBUG("Sent less data than expected!");
        }
        file.close();
        return true;
    }

    return false;
}

String lastExistingParent(String path) {
  while (!path.isEmpty() && !fileSystem->exists(path)) {
    if (path.lastIndexOf('/') > 0) {
      path = path.substring(0, path.lastIndexOf('/'));
    } else {
      path = String();  // No slash => the top folder does not exist
    }
  }
  print_DEBUG(String("Last existing parent: ") + path);
  return path;
}

void handleFileCreate() {
  String path = Wserver.arg("path");
  if (path.isEmpty()) {
    return replyBadRequest(F("PATH ARG MISSING"));
  }
  if (path == "/") {
    return replyBadRequest("BAD PATH");
  }
  if (fileSystem->exists(path)) {
    return replyBadRequest(F("PATH FILE EXISTS"));
  }

  String src = Wserver.arg("src");
  if (src.isEmpty()) {
    // No source specified: creation
    print_DEBUG(String("handleFileCreate: ") + path);
    if (path.endsWith("/")) {
      // Create a folder
      path.remove(path.length() - 1);
      if (!fileSystem->mkdir(path)) {
        return replyServerError(F("MKDIR FAILED"));
      }
    } else {
      // Create a file
      File file = fileSystem->open(path, "w");
      if (file) {
        file.write((const char *)0);
        file.close();
      } else {
        return replyServerError(F("CREATE FAILED"));
      }
    }
    if (path.lastIndexOf('/') > -1) {
      path = path.substring(0, path.lastIndexOf('/'));
    }
    replyOKWithMsg(path);
  } else {
    // Source specified: rename
    if (src == "/") {
      return replyBadRequest("BAD SRC");
    }
    if (!fileSystem->exists(src)) {
      return replyBadRequest(F("SRC FILE NOT FOUND"));
    }
    print_DEBUG(String("handleFileCreate: ") + path + " from " + src);
    if (path.endsWith("/")) {
      path.remove(path.length() - 1);
    }
    if (src.endsWith("/")) {
      src.remove(src.length() - 1);
    }
    if (!fileSystem->rename(src, path)) {
      return replyServerError(F("RENAME FAILED"));
    }
    replyOKWithMsg(lastExistingParent(src));
  }
}

void deleteRecursive(String path) {
  File file = fileSystem->open(path, "r");
  bool isDir = file.isDirectory();
  file.close();

  // If it's a plain file, delete it
  if (!isDir) {
    fileSystem->remove(path);
    return;
  }

  // Otherwise delete its contents first
  Dir dir = fileSystem->openDir(path);

  while (dir.next()) {
    deleteRecursive(path + '/' + dir.fileName());
  }

  // Then delete the folder itself
  fileSystem->rmdir(path);
}

void handleFileDelete() {
  String path = Wserver.arg(0);
  if (path.isEmpty() || path == "/") {
    return replyBadRequest("BAD PATH");
  }
  print_DEBUG(String("handleFileDelete: ") + path);
  if (!fileSystem->exists(path)) {
    return replyNotFound(FPSTR(FILE_NOT_FOUND));
  }
  deleteRecursive(path);

  replyOKWithMsg(lastExistingParent(path));
}

void handleFileUpload() {

  if (Wserver.uri() != "/edit") {
    return;
  }
  HTTPUpload& upload = Wserver.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    // Make sure paths always start with "/"
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    print_DEBUG(String("handleFileUpload Name: ") + filename);
    uploadFile = fileSystem->open(filename, "w");
    if (!uploadFile) {
      return replyServerError(F("CREATE FAILED"));
    }
    print_DEBUG(String("Upload: START, filename: ") + filename);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      size_t bytesWritten = uploadFile.write(upload.buf, upload.currentSize);
      if (bytesWritten != upload.currentSize) {
        return replyServerError(F("WRITE FAILED"));
      }
    }
    print_DEBUG(String("Upload: WRITE, Bytes: ") + upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
    }
    print_DEBUG(String("Upload: END, Size: ") + upload.totalSize);
  }
}

void handleNotFound() {
  String uri = ESP8266WebServer::urlDecode(Wserver.uri()); // required to read paths with blanks

  if (handleFileRead(uri)) {
    return;
  }

  // Dump debug data
  String message;
  message.reserve(100);
  message = F("Error: File not found\n\nURI: ");
  message += uri;
  message += F("\nMethod: ");
  message += (Wserver.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += Wserver.args();
  message += '\n';
  for (uint8_t i = 0; i < Wserver.args(); i++) {
    message += F(" NAME:");
    message += Wserver.argName(i);
    message += F("\n VALUE:");
    message += Wserver.arg(i);
    message += '\n';
  }
  message += "path=";
  message += Wserver.arg("path");
  message += '\n';
  print_DEBUG(message);
  return replyNotFound(message);
}

void handleGetEdit() {
//  if (handleFileRead(F("/edit/index.htm"))) {
//    return;
//  }
    Wserver.sendHeader("Cache-Control", "max-age=31536000", false);
    Wserver.sendHeader("Content-Encoding", "gzip");
    Wserver.send_P(200, "text/html", EDIT_PAGE, sizeof(EDIT_PAGE));
//  replyNotFound(FPSTR(FILE_NOT_FOUND));
}


#ifdef USE_WEBSOCKET
WebSocketsServer webSocket = WebSocketsServer(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
String payloadString;
  switch (type) {
    case WStype_DISCONNECTED:
      print_DEBUG("Disconnected:" +  num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        DBG_OUTPUT_PORT.printf("[%u] Connected from %s url: %s\n", num, ip.toString().c_str(), payload);
        // send message to client
        //webSocket.sendTXT(num, "Connected to Serial on " + WiFi.localIP().toString() + "\n");
      }
      break;
    case WStype_TEXT:
      payloadString=(const char *)payload;
      DBG_OUTPUT_PORT.printf("[%u] get Text: %s\n", num, payload);
      print_DEBUG("GET MSG:" + payloadString);
      break;
    case WStype_BIN:
      DBG_OUTPUT_PORT.printf("[%u] get binary lenght: %u\n", num, lenght);
      break;
  }

}
#endif

void print_DEBUG(String msg) {
  #ifdef DEBUG
      DBG_OUTPUT_PORT.println(msg);
  #endif

  if(wifi_status == true ) {
    #ifdef USE_WEBSOCKET
      webSocket.broadcastTXT(msg);
    #endif
    }
}

void web_task_callback(void) {
    if(WEB_task.isFirstIteration()) {
        Wserver.on("/status", HTTP_GET, handleStatus);
        Wserver.on("/favicon.ico", HTTP_GET, []() {
            Wserver.sendHeader("Cache-Control", "max-age=31536000", false);
            Wserver.sendHeader("Content-Encoding", "gzip");
            Wserver.send_P(200, "image/x-icon", FAV_ICON, sizeof(FAV_ICON));
        });
        // List directory
        Wserver.on("/list", HTTP_GET, handleFileList);
        // Load editor
        Wserver.on("/edit", HTTP_GET, handleGetEdit);
        // Create file
        Wserver.on("/edit",  HTTP_PUT, handleFileCreate);
        // Delete file
        Wserver.on("/edit",  HTTP_DELETE, handleFileDelete);
        // Upload file
        // - first callback is called after the request has ended with all parsed arguments
        // - second callback handles file upload at that location
        Wserver.on("/edit",  HTTP_POST, replyOK, handleFileUpload);
        // Default handler for all URIs not defined above
        // Use it to read files from filesystem
        Wserver.onNotFound(handleNotFound);


#ifdef USE_WEBSOCKET
        webSocket.begin();
        webSocket.onEvent(webSocketEvent);
        Wserver.on("/debug", HTTP_GET, []() {
          if(WiFiSettings.secure == true) {
              if (!Wserver.authenticate("admin", WiFiSettings.password.c_str())) {
              return Wserver.requestAuthentication();
            } 
          }
          Wserver.sendHeader("Cache-Control", "max-age=31536000", false);
          Wserver.sendHeader("Content-Encoding", "gzip");
          Wserver.send_P(200, "text/html", WD_PAGE, sizeof(WD_PAGE));
        });
#endif
        Wserver.begin();
    }
    Wserver.handleClient();
#ifdef USE_WEBSOCKET
    webSocket.loop();
#endif
}

void init_web(Scheduler *sc) {

    sc->addTask(WEB_task);
    WEB_task.enable();
}
