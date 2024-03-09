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

#ifndef ACORN_NETWORK_EPOLL_H
#define ACORN_NETWORK_EPOLL_H

class acorn_epoll {
    int _mepollfd = -1;

 public:   
    std::unordered_set<int> _fd_master_pool = {};
    std::unordered_set<int> _fd_ready_client_pool = {};

    acorn_epoll() {}

    void acorn_createEpoll() {
        _mepollfd = epoll_create1(0);
        if (_mepollfd == -1) {
            throw std::runtime_error(std::string(strerror(errno)));
        }
        std::cout << "epoll instance created" << std::endl;
    }

    void acorn_epollAddMSocket(std::vector<int> &msocks) {
        for (const auto& mfd : msocks) {
            struct epoll_event event;
            event.events = EPOLLIN | EPOLLET;
            event.data.fd = mfd;
            if (epoll_ctl(_mepollfd, EPOLL_CTL_ADD, mfd, &event) == -1) {
                throw std::runtime_error(std::string(strerror(errno)));
            }
            _fd_master_pool.insert(mfd);
            std::cout << "Socket: [" << mfd << "] added to epoll instance" << std::endl;
        }
    }

    void acorn_epollEventsReady() {
        constexpr int MAX_EVENTS = 128;
        struct epoll_event events[MAX_EVENTS];

        int eventWait = epoll_wait(_mepollfd, events, MAX_EVENTS, -1);
        if (eventWait == -1) {
            throw std::runtime_error(std::string(strerror(errno)));
        }

        for (int i = 0; i < eventWait; ++i) {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || events[i].data.fd == -1) {
                if (events[i].data.fd != -1) {
                    shutdown(events[i].data.fd, SHUT_RDWR);
                    close(events[i].data.fd);
                    _fd_ready_client_pool.erase(events[i].data.fd);
                }
                std::cout << "Closed Read and Write for local cleint socket " << events[i].data.fd << std::endl;
                continue;
            }

            if ((events[i].events & EPOLLRDHUP) || events[i].data.fd == -1) {
                if (events[i].data.fd != -1) {
                    shutdown(events[i].data.fd, SHUT_RD);
                    _fd_ready_client_pool.erase(events[i].data.fd);
                }
                std::cout << "Remote Client Socket has closed sending " << events[i].data.fd << std::endl;
                continue;
            }

            if (events[i].events & EPOLLIN) {
                if (_fd_master_pool.find(events[i].data.fd) != _fd_master_pool.end()) {
                    const int cfd = accept4(events[i].data.fd, nullptr, nullptr, SOCK_NONBLOCK);
                    if (cfd == -1) {
                        throw std::runtime_error(std::string(strerror(errno)));
                    }

                    struct epoll_event event;
                    event.events = EPOLLIN | EPOLLET;
                    event.data.fd = cfd;
                    if (epoll_ctl(_mepollfd, EPOLL_CTL_ADD, cfd, &event) == -1) {
                        throw std::runtime_error(std::string(strerror(errno)));
                    }
                    std::cout << "cleint socket added: " << cfd << std::endl;
                } else {
                    _fd_ready_client_pool.insert(events[i].data.fd);
                    acorn_http http;
                    http.acorn_http_workers(events[i].data.fd);
                } 
            }    
        }
        
        if(_fd_ready_client_pool.size() > 0) {
        } 
    }
      
    ~acorn_epoll() {
        for (auto cfd : _fd_ready_client_pool) {
            if(cfd != -1) {
                close(cfd);
                std::cout << "Client closed successfully: " << cfd << std::endl;
            }
        }

        if (_mepollfd != -1) {
            close(_mepollfd);
            std::cout << "Epoll instance closed successfully: " << _mepollfd << std::endl;
        }
    }
};

#endif