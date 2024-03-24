/*
*   Copyright @ 2024 Acorn
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

#ifndef ACORN_NETWORK_EPOLL_H
#define ACORN_NETWORK_EPOLL_H

const constexpr int EPOLL_MAX_EVENTS = 128;
constexpr const ssize_t MIN_BUFFER_SIZE = 1024;

class acorn_epoll {
    int _mepollfd = -1;
    std::unordered_set<int> _fd_master_pool = {};     
    std::unordered_map<int, std::string> _fd_ready_client_pool = {};

public: 
    acorn_epoll() {}

    void acorn_createEpoll() {
        _mepollfd = epoll_create1(0);
        if (_mepollfd == -1) {
            const int err = errno;
            throw std::runtime_error("Failed to create epoll instance: " + std::string(strerror(err)));
        }
    }

    void acorn_epollAddMSocket(std::vector<int> &msocks) {
        for (const auto& mfd : msocks) {
            struct epoll_event event;
            event.events = EPOLLIN | EPOLLET;
            event.data.fd = mfd;
            if (epoll_ctl(_mepollfd, EPOLL_CTL_ADD, mfd, &event) == -1) {
                const int err = errno;
                throw std::runtime_error("epoll_ctl failed: " + std::string(strerror(err)));
            }
            _fd_master_pool.insert(mfd);
        }
    }

    void acorn_epollGraceClose(int cfd, const int& type) {
        try {
            auto it = _fd_ready_client_pool.find(cfd);
            if (it != _fd_ready_client_pool.end()) {
                _fd_ready_client_pool.erase(it);
                if (type == SHUT_RDWR || type == SHUT_RD || type == SHUT_WR) {
                    if(shutdown(cfd, type) == -1) { // Shutdown failed
                        const int err = errno;
                        throw std::runtime_error("Failed to shutdown socket: " + std::string(strerror(err)));
                    }
                }
                if(type == SHUT_RDWR) {
                    while(true) {    
                        if(close(cfd) == -1) { // Close failed
                            const int err = errno;
                            if(err == EINTR) { // The call was interrupted by a signal handler before any events were received.
                                continue;
                            } else {
                                throw std::runtime_error("Unexpected error while closing file descriptor: " + std::string(strerror(err)));
                            }
                        } else {
                            break;
                        }  
                    } 
                }
            }
        } 
        catch (const std::runtime_error& e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void acorn_epollEventsReady() {
        while(running) {
            struct epoll_event events[EPOLL_MAX_EVENTS];
            int eventCount = epoll_wait(_mepollfd, events, EPOLL_MAX_EVENTS, -1);
            if (eventCount == -1) {
                const int err = errno;
                if(err == EINTR) { // The call was interrupted by a signal handler before any events were received.
                    continue;
                } else {
                    throw std::runtime_error("Failed to epoll wait: " + std::string(strerror(err)));
                }
            }

            for (int i = 0; i < eventCount; ++i) {
                
                if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                    acorn_epollGraceClose(events[i].data.fd, SHUT_RDWR);
                    std::cerr << "Error or hang-up on socket: " << events[i].data.fd << std::endl;
                }
                
                if ((events[i].events & EPOLLIN) && !(events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))) {
                    if (_fd_master_pool.find(events[i].data.fd) != _fd_master_pool.end()) {
                        const int clientfd = accept4(events[i].data.fd, nullptr, nullptr, SOCK_NONBLOCK);
                        if (clientfd == -1) {
                            const int err = errno;
                            if (err == EBADF || err == EFAULT || err == EINVAL || err == EMFILE || err == ENFILE || 
                                      err == ENOBUFS || err == ENOMEM || err == ENOTSOCK || err == EOPNOTSUPP || err == EPROTO || 
                                      err == EPERM) {
                                throw std::runtime_error("Failed to accept new connection: " + std::string(strerror(err)));
                            }
                            continue;
                        }

                        struct epoll_event event = {EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET, {.fd = clientfd}};
                        if (epoll_ctl(_mepollfd, EPOLL_CTL_ADD, clientfd, &event) == -1) {
                            const int err = errno;
                            if (err == EEXIST || err == EBADF || err == EINVAL || err == ELOOP || err == ENOENT || err == ENOMEM || err == ENOSPC || err == EPERM) {
                                acorn_epollGraceClose(clientfd, SHUT_RDWR);
                                throw std::runtime_error("epoll_ctl failed: " + std::string(strerror(err)));
                            }
                            acorn_epollGraceClose(clientfd, SHUT_RDWR);
                            continue;
                        } 
                        _fd_ready_client_pool.insert({clientfd, ""});
                    } else {
                        const int cfd = events[i].data.fd;

                        std::cout << "EPOLLIN EVENT" << std::endl;
                        try {
                            char temp_buffer[MIN_BUFFER_SIZE];
                            while (true) {
                                memset(temp_buffer, 0, MIN_BUFFER_SIZE);
                                ssize_t bytesRead = recv(cfd, temp_buffer, sizeof(temp_buffer), 0);
                                if (bytesRead > 0) {
                                    _fd_ready_client_pool[cfd].append(temp_buffer, bytesRead);
                                } else if (bytesRead == 0) {
                                    // We choosing EPOLLRDHUP to handle client closed connection
                                    break;
                                } else if(bytesRead == -1) {
                                    const int err = errno;
                                    if(err == EINTR) { // Interrupted by a signal before any data was received
                                        continue;
                                    } else if (err == EAGAIN || err == EWOULDBLOCK) { // Non-blocking socket operation should be retried later as no data is available now
                                        break;
                                    } else if (err == EBADF || err == EFAULT || err == EINVAL || err == ENOMEM || err == ENOTCONN || err == ENOTSOCK){
                                        throw std::runtime_error("Receive failed: " + std::string(strerror(err)));
                                    }
                                    break;
                                }
                            }

                            std::cout << _fd_ready_client_pool[cfd] << std::endl;
                        }
                        catch (const std::runtime_error& e) {
                            acorn_epollGraceClose(cfd, SHUT_RDWR);
                            std::cerr << e.what() << std::endl;
                        }
                    }
                }

                if ((events[i].events & EPOLLOUT) && !(events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))) {
                    const int cfd = events[i].data.fd;

                    std::cout << "EPOLLOUT EVENT" << std::endl;
                    try {         
                        if (!_fd_ready_client_pool[cfd].empty()) {
                            std::string_view httpRequest(_fd_ready_client_pool[cfd]);
                            std::ostringstream responseStream = acorn_header_parser(httpRequest);
                            std::string response = responseStream.str();
        
                            if (!response.empty()) {

                                std::cout << response << std::endl;

                                size_t totalBytesSent = 0;
                                size_t responseLength = response.length();
                                const char* responseData = response.c_str();

                                while (totalBytesSent < responseLength) {
                                    ssize_t bytesSent = send(cfd, responseData + totalBytesSent, responseLength - totalBytesSent, 0);
                                    if (bytesSent == -1) {
                                        const int err = errno;
                                        if (err == EAGAIN || err == EWOULDBLOCK) {
                                            continue;
                                        } else {
                                            break; 
                                        }
                                    }
                                    totalBytesSent += bytesSent;
                                }
                            }
                        }
                    } 
                    catch (const std::runtime_error& e) {
                        acorn_epollGraceClose(cfd, SHUT_RDWR);
                        std::cerr << e.what() << std::endl;
                    }  
                }
            }
        }  
    }
      
    ~acorn_epoll() {
        for (const auto& pair : _fd_ready_client_pool) {
            close(pair.first);
            std::cout << "Closing Client fd: " << pair.first << std::endl;
        }
        if (_mepollfd != -1) {
            close(_mepollfd);
            std::cout << "Epoll instance closed successfully" << std::endl;
        }
    }
};
#endif