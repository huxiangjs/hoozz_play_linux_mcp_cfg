/*
 * MIT License
 *
 * Copyright (c) 2026 huxiangjs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <iostream>
#include <unistd.h>
#include "event_bus.h"
#include "simple_ctrl.h"
#include "store.h"

static int linux_dev_request(char *buffer, int buf_offs, int vaild_size, int buff_size)
{
	(void)buffer;
	(void)buf_offs;
	(void)vaild_size;
	(void)buff_size;

	return 0;
}

static bool app_event_notify_callback(struct event_bus_msg *msg)
{
	switch (msg->type) {
	case EVENT_BUS_STARTUP:
		break;
	}

	return false;
}

int main(int argc, char *argv[])
{
	struct event_bus_msg msg = { EVENT_BUS_STARTUP, 0, 0};

	(void)argc;
	(void)argv;

	std::cout << "Hello Hoozz Play" << std::endl;

	store_init();

	/* Event bus */
	event_bus_init();
	event_bus_register(app_event_notify_callback);
	event_bus_send(&msg);

	/* Network ctrl */
	simple_ctrl_init();
	simple_ctrl_set_name("LINUX MCP CFG");
	simple_ctrl_set_class_id(CLASS_ID_MCP_CFG);
	simple_ctrl_request_register(linux_dev_request);

	/* Network ready */
	msg.type = EVENT_BUS_WIFI_CONNECTED;
	event_bus_send(&msg);

	while (1)
		usleep(10000);

	return 0;
}
