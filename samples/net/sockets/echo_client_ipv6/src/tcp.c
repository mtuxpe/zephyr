/* tcp.c - TCP specific code for echo client */

/*
 * Copyright (c) 2017 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(net_echo_client_sample, LOG_LEVEL_DBG);

#include <zephyr.h>
#include <errno.h>
#include <stdio.h>

#include <net/socket.h>
#include <net/tls_credentials.h>

#include "common.h"
#include "ca_certificate.h"

#define RECV_BUF_SIZE 128

//static char buf[RECV_BUF_SIZE];

static ssize_t sendall(int sock, const void *buf, size_t len)
{
	while (len) {
		ssize_t out_len = send(sock, buf, len, 0);

		if (out_len < 0) {
			return out_len;
		}
		buf = (const char *)buf + out_len;
		len -= out_len;
	}

	return 0;
}

int send_tcp_data(struct data *data)
{
	int ret;

	do {
		data->tcp.expecting = sys_rand32_get() % ipsum_len;
	} while (data->tcp.expecting == 0);

	data->tcp.received = 0U;

	ret =  sendall(data->tcp.sock, lorem_ipsum, data->tcp.expecting);

	if (ret < 0) {
		LOG_ERR("%s TCP: Failed to send data, errno %d", data->proto,
			errno);
	} else {
		LOG_DBG("%s TCP: Sent %d bytes", data->proto,
			data->tcp.expecting);
	}

	return ret;
}

static int compare_tcp_data(struct data *data, const char *buf, u32_t received)
{
	if (data->tcp.received + received > data->tcp.expecting) {
		LOG_ERR("Too much data received: TCP %s", data->proto);
		return -EIO;
	}

	if (memcmp(buf, lorem_ipsum + data->tcp.received, received) != 0) {
		LOG_ERR("Invalid data received: TCP %s", data->proto);
		return -EIO;
	}

	return 0;
}

static int start_tcp_proto(struct data *data, struct sockaddr *addr,
			   socklen_t addrlen)
{
	int ret;

	data->tcp.sock = socket(addr->sa_family, SOCK_STREAM, IPPROTO_TCP);

	if (data->tcp.sock < 0) {
		LOG_ERR("Failed to create TCP socket (%s): %d", data->proto,
			errno);
		return -errno;
	}

	ret = connect(data->tcp.sock, addr, addrlen);
	if (ret < 0) {
		LOG_ERR("Cannot connect to TCP remote (%s): %d", data->proto,
			errno);
		ret = -errno;
	}

	return ret;
}

static int process_tcp_proto(struct data *data)
{
	int ret, received;
	char buf[RECV_BUF_SIZE];

	do {
		received = recv(data->tcp.sock, buf, sizeof(buf), MSG_DONTWAIT);

		/* No data or error. */
		if (received == 0) {
			ret = -EIO;
			continue;
		} else if (received < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				ret = 0;
			} else {
				ret = -errno;
			}
			continue;
		}

		ret = compare_tcp_data(data, buf, received);
		if (ret != 0) {
			break;
		}

		/* Successful comparison. */
		data->tcp.received += received;
		if (data->tcp.received < data->tcp.expecting) {
			continue;
		}

		/* Response complete */
		LOG_DBG("%s TCP: Received and compared %d bytes, all ok",
			data->proto, data->tcp.received);


		if (++data->tcp.counter % 1000 == 0) {
			LOG_INF("%s TCP: Exchanged %u packets", data->proto,
				data->tcp.counter);
		}

        k_sleep(1000);
	    ret = send_tcp_data(data);
	    break;
	} while (received > 0);

	return ret;
}

int start_tcp(void)
{
	int ret = 0;
	struct sockaddr_in6 addr6;

    LOG_INF("Call start tcp proto");

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(PEER_PORT);
		inet_pton(AF_INET6, CONFIG_NET_CONFIG_PEER_IPV6_ADDR,
			  &addr6.sin6_addr);

		ret = start_tcp_proto(&conf.ipv6, (struct sockaddr *)&addr6,
				      sizeof(addr6));
		if (ret < 0) {
			return ret;
		}
	}

    LOG_INF("Call send TCP data");

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		ret = send_tcp_data(&conf.ipv6);
		if (ret < 0) {
			return ret;
		}
	}

	return ret;
}

int process_tcp(void)
{
	int ret = 0;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		ret = process_tcp_proto(&conf.ipv6);
		if (ret < 0) {
			return ret;
		}
	}

	return ret;
}

void stop_tcp(void)
{
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		if (conf.ipv6.tcp.sock >= 0) {
			(void)close(conf.ipv6.tcp.sock);
		} else {
		   LOG_INF("Close err %x",conf.ipv6.tcp.sock );
		}
	}

}
