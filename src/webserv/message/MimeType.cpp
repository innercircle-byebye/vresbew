#include "MimeType.hpp"

namespace ft {

const std::map<std::string, std::string> MimeType::mime_types_ = MimeType::makeMimeTypes();

std::map<std::string, std::string> MimeType::makeMimeTypes(void) {
	std::map<std::string, std::string> types;
	types["EMPTY"] = "";
  types[".txt"] = "text/plain";
	types[".bin"] = "application/octet-stream";
	types[".jpeg"] = "image/jpeg";
	types[".jpg"] = "image/jpeg";
	types[".html"] = "text/html";
	types[".htm"] = "text/html";
	types[".png"] = "image/png";
	types[".bmp"] = "image/bmp";
	types[".gif"] = "image/gif";
	types[".pdf"] = "application/pdf";
	types[".tar"] = "application/x-tar";
	types[".json"] = "application/json";
	types[".css"] = "text/css";
	types[".js"] = "application/javascript";
	types[".mp3"] = "audio/mpeg";
	types[".avi"] = "video/x-msvideo";
	types[".php"] = "text/html";
	return types;
}

MimeType::MimeType(void) {}

const std::string &MimeType::of(std::string& extention) {
	std::map<std::string, std::string>::const_iterator it = mime_types_.find(extention);
	if (it != mime_types_.end())
		return it->second;
	return mime_types_.find("EMPTY")->second;
}

}  // namespace ft
