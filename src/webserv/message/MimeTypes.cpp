#include "MimeTypes.hpp"

ft::MimeTypes::MimeTypes() {
  this->initMimeTypes();
}

ft::MimeTypes::~MimeTypes() {}

void ft::MimeTypes::initMimeTypes() {
  this->mime_types_[".txt"] = "text/plain";
	this->mime_types_[".bin"] = "application/octet-stream";
	this->mime_types_[".jpeg"] = "image/jpeg";
	this->mime_types_[".jpg"] = "image/jpeg";
	this->mime_types_[".html"] = "text/html";
	this->mime_types_[".htm"] = "text/html";
	this->mime_types_[".png"] = "image/png";
	this->mime_types_[".bmp"] = "image/bmp";
	this->mime_types_[".gif"] = "image/gif";
	this->mime_types_[".pdf"] = "application/pdf";
	this->mime_types_[".tar"] = "application/x-tar";
	this->mime_types_[".json"] = "application/json";
	this->mime_types_[".css"] = "text/css";
	this->mime_types_[".js"] = "application/javascript";
	this->mime_types_[".mp3"] = "audio/mpeg";
	this->mime_types_[".avi"] = "video/x-msvideo";
}

const char *ft::MimeTypes::getContentType(const char *extension) const {
  return ((this->mime_types_.find(extension))->second);
}
