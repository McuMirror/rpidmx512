/**
 * @file networktcp.cpp
 *
 */
/* Copyright (C) 2021-2023 by Arjan van Vught mailto:info@orangepi-dmx.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <cstdio>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <cassert>

#include "network.h"

// https://cboard.cprogramming.com/c-programming/158125-sockets-using-poll.html

#define MAX_PORTS_ALLOWED		2
#define MAX_SEGMENT_LENGTH		1400

static uint16_t s_ports_allowed[MAX_PORTS_ALLOWED];
static struct pollfd poll_set[MAX_PORTS_ALLOWED][4];
static int server_sockfd[MAX_PORTS_ALLOWED];
static uint8_t s_ReadBuffer[MAX_SEGMENT_LENGTH];

int32_t Network::TcpBegin(uint16_t nLocalPort) {
	int32_t i;

	for (i = 0; i < MAX_PORTS_ALLOWED; i++) {
		if (s_ports_allowed[i] == nLocalPort) {
			perror("TcpBegin: connection already exists");
			return -2;
		}

		if (s_ports_allowed[i] == 0) {
			break;
		}
	}

	if (i == MAX_PORTS_ALLOWED) {
		perror("TcpBegin: too many connections");
		return -1;
	}

	s_ports_allowed[i] = nLocalPort;

	memset(&poll_set[i], 0, sizeof(poll_set));

	server_sockfd[i] = socket(AF_INET, SOCK_STREAM, 0);

	if (server_sockfd[i] == -1) {
		perror("Could not create socket");
		return -1;
	}

    int flag = 1;
    if (-1 == setsockopt(server_sockfd[i], SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag))) {
        perror("setsockopt fail");
    }

	struct sockaddr_in server;

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(nLocalPort);

	if (bind(server_sockfd[i], (struct sockaddr*) &server, sizeof(server)) < 0) {
		perror("bind failed");
		printf(IPSTR ":%d\n", IP2STR(server.sin_addr.s_addr), nLocalPort);
		return -2;
	}

	listen(server_sockfd[i], 0);

    poll_set[i][0].fd = server_sockfd[i];
    poll_set[i][0].events = POLLIN | POLLPRI;

    printf("Network::TcpBegin -> i=%d\n", i);
	return i;
}

int32_t Network::TcpEnd(const int32_t nHandle) {
	assert(nHandle < MAX_PORTS_ALLOWED);

	close(poll_set[nHandle][0].fd);
	close(poll_set[nHandle][1].fd);

	return -1;
}

uint16_t Network::TcpRead(const int32_t nHandle, const uint8_t **ppBuffer,uint32_t &HandleConnection) {
	assert(nHandle < MAX_PORTS_ALLOWED);

	const int poll_result = poll(poll_set[nHandle], 2, 0);

	if (poll_result <= 0) {
		return poll_result;
	}

	for (int fd_index = 0; fd_index < 2; fd_index++) {
		if (poll_set[nHandle][fd_index].revents & POLLIN) {
			if (poll_set[nHandle][fd_index].fd == server_sockfd[nHandle]) {
				struct sockaddr_in client_address;
				int client_len = sizeof(struct sockaddr_in);

				const int client_sockfd = accept(server_sockfd[nHandle], (struct sockaddr*) &client_address, (socklen_t*) &client_len);

				if (client_sockfd < 0) {
					perror("accept failed");
					return client_sockfd;
				}

				poll_set[nHandle][1].fd = client_sockfd;
				poll_set[nHandle][1].events = POLLIN | POLLPRI;

				printf("Adding client on fd %d\n", client_sockfd);
			} else {
				int nread;
				ioctl(poll_set[nHandle][fd_index].fd, FIONREAD, &nread);

				if (nread == 0) {
					printf("Removing client on fd %d\n", poll_set[nHandle][fd_index].fd);
					close(poll_set[nHandle][fd_index].fd);
					poll_set[nHandle][fd_index].fd = 0;
					poll_set[nHandle][fd_index].events = 0;
					poll_set[nHandle][fd_index].revents = 0;
				} else {
					printf("Serving client on fd %d\n", poll_set[nHandle][fd_index].fd);
					const int bytes = read(poll_set[nHandle][fd_index].fd, s_ReadBuffer, MAX_SEGMENT_LENGTH);
					if (bytes <= 0) {
						perror("read failed");
						close(poll_set[nHandle][fd_index].fd);
						poll_set[nHandle][fd_index].fd = 0;
						poll_set[nHandle][fd_index].events = 0;
						poll_set[nHandle][fd_index].revents = 0;
					} else {
						HandleConnection = static_cast<uint32_t>(poll_set[nHandle][fd_index].fd);
						*ppBuffer = reinterpret_cast<uint8_t*>(&s_ReadBuffer);
						return static_cast<uint16_t>(bytes);
					}
				}
			}

		}

	}

	return 0;
}

void Network::TcpWrite( __attribute__((unused)) const int32_t nHandle, const uint8_t *pBuffer, uint16_t nLength, const uint32_t HandleConnection) {
	assert(nHandle < MAX_PORTS_ALLOWED);

	printf("Write client on fd %d\n", HandleConnection);

	const int c = write(static_cast<int>(HandleConnection), pBuffer, nLength);

	if (c < 0) {
		perror("write");
	}
}
