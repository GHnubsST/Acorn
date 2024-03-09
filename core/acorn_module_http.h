/*
*   Copyright 2024 Acorn
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*/


#ifndef ACORN_HTTP_H
#define ACORN_HTTP_H

// Informational responses
constexpr const char* HTTP_100 = "HTTP/1.1 100 Continue";
constexpr const char* HTTP_101 = "HTTP/1.1 101 Switching Protocols";
constexpr const char* HTTP_102 = "HTTP/1.1 102 Processing";
constexpr const char* HTTP_103 = "HTTP/1.1 103 Early Hints";

// Successful responses
constexpr const char* HTTP_200 = "HTTP/1.1 200 OK";
constexpr const char* HTTP_201 = "HTTP/1.1 201 Created";
constexpr const char* HTTP_202 = "HTTP/1.1 202 Accepted";
constexpr const char* HTTP_203 = "HTTP/1.1 203 Non-Authoritative Information";
constexpr const char* HTTP_204 = "HTTP/1.1 204 No Content";
constexpr const char* HTTP_205 = "HTTP/1.1 205 Reset Content";
constexpr const char* HTTP_206 = "HTTP/1.1 206 Partial Content";
constexpr const char* HTTP_207 = "HTTP/1.1 207 Multi-Status";
constexpr const char* HTTP_208 = "HTTP/1.1 208 Already Reported";
constexpr const char* HTTP_226 = "HTTP/1.1 226 IM Used";

// Redirection messages
constexpr const char* HTTP_300 = "HTTP/1.1 300 Multiple Choices";
constexpr const char* HTTP_301 = "HTTP/1.1 301 Moved Permanently";
constexpr const char* HTTP_302 = "HTTP/1.1 302 Found";
constexpr const char* HTTP_303 = "HTTP/1.1 303 See Other";
constexpr const char* HTTP_304 = "HTTP/1.1 304 Not Modified";
constexpr const char* HTTP_305 = "HTTP/1.1 305 Use Proxy";
constexpr const char* HTTP_307 = "HTTP/1.1 307 Temporary Redirect";
constexpr const char* HTTP_308 = "HTTP/1.1 308 Permanent Redirect";

// Client error responses
constexpr const char* HTTP_400 = "HTTP/1.1 400 Bad Request";
constexpr const char* HTTP_401 = "HTTP/1.1 401 Unauthorized";
constexpr const char* HTTP_402 = "HTTP/1.1 402 Payment Required";
constexpr const char* HTTP_403 = "HTTP/1.1 403 Forbidden";
constexpr const char* HTTP_404 = "HTTP/1.1 404 Not Found";
constexpr const char* HTTP_405 = "HTTP/1.1 405 Method Not Allowed";
constexpr const char* HTTP_406 = "HTTP/1.1 406 Not Acceptable";
constexpr const char* HTTP_407 = "HTTP/1.1 407 Proxy Authentication Required";
constexpr const char* HTTP_408 = "HTTP/1.1 408 Request Timeout";
constexpr const char* HTTP_409 = "HTTP/1.1 409 Conflict";
constexpr const char* HTTP_410 = "HTTP/1.1 410 Gone";
constexpr const char* HTTP_411 = "HTTP/1.1 411 Length Required";
constexpr const char* HTTP_412 = "HTTP/1.1 412 Precondition Failed";
constexpr const char* HTTP_413 = "HTTP/1.1 413 Payload Too Large";
constexpr const char* HTTP_414 = "HTTP/1.1 414 URI Too Long";
constexpr const char* HTTP_415 = "HTTP/1.1 415 Unsupported Media Type";
constexpr const char* HTTP_416 = "HTTP/1.1 416 Range Not Satisfiable";
constexpr const char* HTTP_417 = "HTTP/1.1 417 Expectation Failed";
constexpr const char* HTTP_418 = "HTTP/1.1 418 I'm a teapot";
constexpr const char* HTTP_421 = "HTTP/1.1 421 Misdirected Request";
constexpr const char* HTTP_422 = "HTTP/1.1 422 Unprocessable Entity";
constexpr const char* HTTP_423 = "HTTP/1.1 423 Locked";
constexpr const char* HTTP_424 = "HTTP/1.1 424 Failed Dependency";
constexpr const char* HTTP_425 = "HTTP/1.1 425 Too Early";
constexpr const char* HTTP_426 = "HTTP/1.1 426 Upgrade Required";
constexpr const char* HTTP_428 = "HTTP/1.1 428 Precondition Required";
constexpr const char* HTTP_429 = "HTTP/1.1 429 Too Many Requests";
constexpr const char* HTTP_431 = "HTTP/1.1 431 Request Header Fields Too Large";
constexpr const char* HTTP_451 = "HTTP/1.1 451 Unavailable For Legal Reasons";

// Server error responses
constexpr const char* HTTP_500 = "HTTP/1.1 500 Internal Server Error";
constexpr const char* HTTP_501 = "HTTP/1.1 501 Not Implemented";
constexpr const char* HTTP_502 = "HTTP/1.1 502 Bad Gateway";
constexpr const char* HTTP_503 = "HTTP/1.1 503 Service Unavailable";
constexpr const char* HTTP_504 = "HTTP/1.1 504 Gateway Timeout";
constexpr const char* HTTP_505 = "HTTP/1.1 505 HTTP Version Not Supported";
constexpr const char* HTTP_506 = "HTTP/1.1 506 Variant Also Negotiates";
constexpr const char* HTTP_507 = "HTTP/1.1 507 Insufficient Storage";
constexpr const char* HTTP_508 = "HTTP/1.1 508 Loop Detected";
constexpr const char* HTTP_510 = "HTTP/1.1 510 Not Extended";
constexpr const char* HTTP_511 = "HTTP/1.1 511 Network Authentication Required";

/*************************************************WORK IN PROGRESS**********************************************************/
class acorn_http {
    int cleint_fd;
    const int MAX_BUFFER_SIZE = 4096;

    std::unordered_map<std::string_view, std::string_view> acorn_http_header_parser(const std::string_view& header) {

        std::unordered_map<std::string_view, std::string_view> header_map;

        size_t firstLineEnd = header.find("\r\n");
        if (firstLineEnd == std::string_view::npos) {
            header_map["HTTP-Code"] = HTTP_400;
            return header_map;
        }

        std::string_view request_line = header.substr(0, firstLineEnd);
        if (request_line.length() > 8000) {
            header_map["HTTP-Code"] = HTTP_414;
            return header_map;
        }
        
        size_t whiteSpaceCount = std::count(request_line.begin(), request_line.end(), ' ');
        if (whiteSpaceCount != 2) {
            header_map["HTTP-Code"] = HTTP_400; 
            return header_map;
        }

        size_t firstSpace = request_line.find(' ');
        size_t secondSpace = request_line.find(' ', firstSpace + 1);
        if (firstSpace == std::string_view::npos || secondSpace == std::string_view::npos) {
            header_map["HTTP-Code"] = HTTP_400;
            return header_map;
        }

        std::string_view method = request_line.substr(0, firstSpace);
        std::string_view request_target = request_line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
        std::string_view version = request_line.substr(secondSpace + 1);

        if (method != "GET" && method != "POST" && method != "HEAD") {
            header_map["HTTP-Code"] = HTTP_405;
            return header_map;
        }

        if (version != "HTTP/1.1" && version != "HTTP/1.0") { 
            header_map["HTTP-Code"] = HTTP_505;
            return header_map;
        }

        size_t start = firstLineEnd + 2;
        while (start < header.size()) {
            size_t end = header.find("\r\n", start);
            if (end == std::string_view::npos || header[start] == '\r') break;

            auto line = header.substr(start, end - start);
            auto colonPos = line.find(':');
            if (colonPos == std::string_view::npos || line.find_first_not_of(" \t") != 0 || line[colonPos - 1] == ' ' || line[colonPos - 1] == '\t') {
                header_map["HTTP-Code"] = HTTP_400;
                return header_map;
            }

            auto key = line.substr(0, colonPos);
            auto value = line.substr(colonPos + 1).substr(line.substr(colonPos + 1).find_first_not_of(" \t"));
            header_map[key] = value;
            start = end + 2;
        }

        header_map["Method"] = method;
        header_map["Request-Target"] = request_target;
        header_map["HTTP-Code"] = HTTP_200;

        return header_map;
    }

    void acorn_http_worker(const int &cfd) {
        ssize_t totalBytesRead = 0;
        ssize_t bytesRead;
        char buffer[MAX_BUFFER_SIZE] = {0}; 

        do {
            bytesRead = recv(cfd, buffer + totalBytesRead, sizeof(buffer) - totalBytesRead - 1, 0);
            if (bytesRead == -1) {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    std::cerr << "Error receiving data: " << strerror(errno) << std::endl;
                    close(cfd);
                }
            } else if (bytesRead == 0) {
                std::cout << "Cant Read Connection closed by client: " << cfd << std::endl;
                close(cfd);
            } else {
                totalBytesRead += bytesRead;
                if (totalBytesRead >= MAX_BUFFER_SIZE - 1) {
                    break;
                }
            }
        } while (bytesRead > 0);

        buffer[totalBytesRead] = '\0';

        std::string_view httpRequest(buffer, totalBytesRead);
        std::string_view header, body;

        auto pos = httpRequest.find("\r\n\r\n");
        if (pos != std::string_view::npos) {
            header = std::string_view(httpRequest.data(), pos);
            body = std::string_view(httpRequest.data() + pos + 4, totalBytesRead - pos - 4);
        } else {
            header = httpRequest;
            body = std::string_view();
        }

        
        auto http_code = acorn_http_header_parser(header);

        for (const auto& [key, value] : http_code) {
            std::cout << key << ":" << value << std::endl;
        }

        std::ostringstream responseStream;
        if(http_code["HTTP-Code"] == HTTP_200) {
            std::time_t currentTime = std::time(nullptr);
            std::tm* currentTm = std::gmtime(&currentTime);
            char timeStr[80];
            std::strftime(timeStr, sizeof(timeStr), "%a, %d %b %Y %H:%M:%S GMT", currentTm);
            std::string content = "<!DOCTYPE html><html lang=\"en\"><head><title>Acorn Web Server</title><meta charset=\"utf-8\"></head><body><h1>Hello, Welcome..</h1></body></html>";
            std::stringstream etagStream;
            etagStream << "\"" << std::hex << std::hash<std::string>{}(content) << "\"";
            std::string etag = etagStream.str();
            responseStream << http_code["HTTP-Code"] << "\r\n";
            responseStream << "Date: " << timeStr << "\r\n";
            responseStream << "Server: Acorn\r\n";
            responseStream << "Last-Modified: " << timeStr << "\r\n";
            responseStream << "ETag: " << etag << "\r\n";
            responseStream << "Accept-Ranges: bytes\r\n";
            responseStream << "Content-Length: " << content.length() << "\r\n";
            responseStream << "Vary: Accept-Encoding\r\n";
            responseStream << "Content-Type: text/html\r\n";
            responseStream << "\r\n";
            responseStream << content << "CRLF.";
        } else {
            responseStream << http_code["HTTP-Code"];
            responseStream << "Server: Acorn\r\n";
            responseStream << "Content-Length: 0" << "\r\n";
        }

        std::string str = responseStream.str();
        char response[str.length() + 1];
        std::strcpy(response, str.c_str());

        ssize_t bytesSent = send(cfd, response, strlen(response), 0);
        if (bytesSent == -1) {
            std::cerr << "Error sending data to client: " << strerror(errno) << std::endl;
        } else {
            std::cout << "Sent data to client " << cfd << std::endl;
        }
    }
public:
    void acorn_http_workers(const int& cfd) {
        cleint_fd = cfd;
        acorn_http_worker(cleint_fd);
    }
};
/*************************************************WORK IN PROGRESS**********************************************************/

/*
    ** RFC7230 **
    *
    * 
    *******acorn_http_header_parser*******
    *   
    *   CHECKS
    *   1. if no CRLF(\r\n) are found in the header set HTTP_CODE to HTTP_400 as its a malformed request
    *   2. Extract and validate the request line, which will set HTTP_400 as a malformed request
    *      rfc7230 - no space or spaces before the method, or in the method, only one space after the method
    *      rfc7230 - no spaces within the request-line only before and after
    *      rfc7230 - no spaces within the HTTP-version only before but not after
    *      rfc7230 - method SP request-target SP HTTP-version CRLF
    *      rfc7231 - url to long HTTP_414 Request-URI too long
    *      rfc7230 - method must be capital letters
    *      rfc7230 - if HTTP version is not HTTP/1.1 then set HTTP_505 HTTP/1.1 505 HTTP Version Not Supported
    *   3. Acorn uses GET,POST,HEAD and sets HTTP_CODE to HTTP_405 method not found for others
    *   4. [key: value] map 
    *      rfc7230 - 3.2.4 no space or spaces before or after keys or within keys otherwise set HTTP_400 as a malformed request
    *      rfc7230 - 3.2.4 if theres no colon then HTTP_400 as a malformed request
    * 
    * 
    * *************************************
*/

#endif