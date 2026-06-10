#include "decoder.hpp"
#include "encoder.hpp"
#include "httplib.hpp"

#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace Logger {

inline void info(const std::string &msg) {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);

  std::cout << "[INFO] " << std::put_time(std::localtime(&time), "%F %T")
            << " - " << msg << '\n';
}

inline void error(const std::string &msg) {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);

  std::cerr << "[ERROR] " << std::put_time(std::localtime(&time), "%F %T")
            << " - " << msg << '\n';
}

} // namespace Logger

namespace {

constexpr const char *STORAGE_DIR = "storage";
constexpr const char *UPLOAD_DIR = "storage/uploads";
constexpr const char *ENCODED_DIR = "storage/encoded";
constexpr const char *DECODED_DIR = "storage/decoded";

std::string readFile(const fs::path &path) {
  std::ifstream ifs(path, std::ios::binary);

  return {std::istreambuf_iterator<char>(ifs),
          std::istreambuf_iterator<char>()};
}

void listFiles(const fs::path &directory, std::vector<std::string> &out) {
  if (!fs::exists(directory))
    return;

  for (const auto &entry : fs::directory_iterator(directory)) {
    out.push_back(entry.path().filename().string());
  }
}

void jsonError(httplib::Response &res, int status, const std::string &message) {
  res.status = status;
  res.set_content("{\"error\":\"" + message + "\"}", "application/json");
}

void jsonSuccess(httplib::Response &res, const std::string &json) {
  res.set_content(json, "application/json");
}

fs::path findFile(const std::string &filename) {
  const std::vector<fs::path> locations = {UPLOAD_DIR, ENCODED_DIR,
                                           DECODED_DIR};

  for (const auto &dir : locations) {
    auto candidate = dir / filename;

    if (fs::exists(candidate))
      return candidate;
  }

  return {};
}

} // namespace

int main() {
  fs::create_directories(STORAGE_DIR);
  fs::create_directories(UPLOAD_DIR);
  fs::create_directories(ENCODED_DIR);
  fs::create_directories(DECODED_DIR);

  httplib::Server svr;

  svr.set_logger([](const httplib::Request &req, const httplib::Response &res) {
    Logger::info(req.method + " " + req.path + " -> " +
                 std::to_string(res.status));
  });

  svr.set_pre_routing_handler(
      [](const httplib::Request &, httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");

        res.set_header("Access-Control-Allow-Headers", "*");

        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");

        return httplib::Server::HandlerResponse::Unhandled;
      });

  svr.Options(R"(.*)", [](const httplib::Request &, httplib::Response &res) {
    res.status = 200;
  });

  // Upload
  svr.Post("/api/upload",
           [](const httplib::Request &req, httplib::Response &res) {
             Logger::info("Received upload request");

             if (!req.form.has_file("file")) {
               Logger::error("Upload request missing file field");

               return jsonError(res, 400, "No file field 'file' in upload");
             }

             const auto &file = req.form.get_file("file");

             if (file.filename.empty()) {
               Logger::error("Upload request has empty filename");

               return jsonError(res, 400, "Empty filename");
             }

             fs::path destination = fs::path(UPLOAD_DIR) / file.filename;

             std::ofstream ofs(destination, std::ios::binary);

             ofs.write(file.content.data(),
                       static_cast<std::streamsize>(file.content.size()));

             Logger::info("Uploaded file: " + file.filename);

             jsonSuccess(res, "{\"status\":\"ok\",\"filename\":\"" +
                                  file.filename + "\"}");
           });

  // List files
  svr.Get("/api/files", [](const httplib::Request &, httplib::Response &res) {
    Logger::info("Listing stored files");

    std::vector<std::string> files;

    listFiles(UPLOAD_DIR, files);
    listFiles(ENCODED_DIR, files);
    listFiles(DECODED_DIR, files);

    std::string json = "[";

    for (size_t i = 0; i < files.size(); ++i) {
      if (i)
        json += ",";

      json += "\"" + files[i] + "\"";
    }

    json += "]";

    res.set_content(json, "application/json");
  });

  // Encode
  svr.Post(R"(/api/encode/(.+))", [](const httplib::Request &req,
                                     httplib::Response &res) {
    std::string filename = req.matches[1];

    fs::path input = fs::path(UPLOAD_DIR) / filename;

    if (!fs::exists(input)) {
      Logger::error("Encode failed. File not found: " + filename);

      return jsonError(res, 404, "File not found: " + filename);
    }

    std::string outputName = filename + ".pgn";

    fs::path output = fs::path(ENCODED_DIR) / outputName;

    try {
      Logger::info("Encoding file: " + filename);

      ChessEncoder{}.encode(input.string(), output.string(), false);

      Logger::info("Encoded successfully: " + outputName);

      jsonSuccess(res, "{\"status\":\"ok\",\"output\":\"" + outputName + "\"}");
    } catch (const std::exception &e) {
      Logger::error(std::string("Encode failed: ") + e.what());

      jsonError(res, 500, e.what());
    }
  });

  // Decode
  svr.Post(R"(/api/decode/(.+))", [](const httplib::Request &req,
                                     httplib::Response &res) {
    std::string filename = req.matches[1];

    fs::path input = fs::path(ENCODED_DIR) / filename;

    if (!fs::exists(input)) {
      Logger::error("Decode failed. File not found: " + filename);

      return jsonError(res, 404, "File not found: " + filename);
    }

    std::string stem = fs::path(filename).stem().string();

    std::string outputName = stem + "_decoded";

    fs::path output = fs::path(DECODED_DIR) / outputName;

    try {
      Logger::info("Decoding file: " + filename);

      ChessDecoder{}.decode(input.string(), output.string(), false);

      Logger::info("Decoded successfully: " + outputName);

      jsonSuccess(res, "{\"status\":\"ok\",\"output\":\"" + outputName + "\"}");
    } catch (const std::exception &e) {
      Logger::error(std::string("Decode failed: ") + e.what());

      jsonError(res, 500, e.what());
    }
  });

  // Download
  svr.Get(R"(/api/download/(.+))",
          [](const httplib::Request &req, httplib::Response &res) {
            std::string filename = req.matches[1];

            auto filepath = findFile(filename);

            if (filepath.empty()) {
              Logger::error("Download failed. File not found: " + filename);

              return jsonError(res, 404, "File not found: " + filename);
            }

            Logger::info("Downloading file: " + filename);

            res.set_content(readFile(filepath), "application/octet-stream");
          });

  Logger::info("Server started at http://localhost:8080");

  svr.listen("0.0.0.0", 8080);

  return 0;
}
