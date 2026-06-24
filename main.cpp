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
#include <fstream>
#include <filesystem>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <pthread.h>
#include "event_bus.h"
#include "simple_ctrl.h"
#include "store.h"

extern "C" const char *get_store_parent_path(void);
static std::string store_parent;

#define DEV_INFO_FILE			"devinfo.txt"
#define DEV_ID_LEN			14
#define DEV_PWD_MAX			16
#define DEV_NUM_MAX			1000

#define SYNC_CMD_GET_COUNT		0x00
#define SYNC_CMD_GET_ITEM		0x01
#define SYNC_CMD_SET_DEV		0x02
#define SYNC_CMD_ADD_DEV		0x03
#define SYNC_CMD_REMOVE_DEV		0x04

#define SYNC_RESULT_OK			0x00
#define SYNC_RESULT_FAIL		0x01
#define SYNC_RESULT_DONE		0x02

#define DEFER_SAVE_MAX			20

struct device_info {
	std::string id;
	std::string password;
	bool vaild;
};

static std::vector<struct device_info> devinfos;
static int defer_save_count;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int app_read_data_file(void)
{
	std::filesystem::path file_path = std::filesystem::path(store_parent) / DEV_INFO_FILE;
	std::ifstream fin(file_path);
	int count = 0;

	if (!fin.is_open()) {
		std::cerr << "Unable to open the file: " << file_path << std::endl;
		return -1;
	}

	std::string line;

	while (std::getline(fin, line)) {
		if (line.length() >= DEV_ID_LEN) {
			device_info info;
			info.id = line.substr(0, DEV_ID_LEN);
			info.password = line.substr(DEV_ID_LEN);
			info.vaild = true;
			devinfos.push_back(info);
			count++;
		} else {
			std::cerr << "Warning: Line length is less than " << DEV_ID_LEN
				<< ": " << line << std::endl;
			return -1;
		}
	}

	fin.close();

#if defined(DEBUG)
	for (const auto& info : devinfos)
		std::cout << "id:" << info.id << ", password:" << info.password << std::endl;
#endif

	std::cout << "Loaded: " << count << " devices" << std::endl;

	return 0;
}

static int app_write_data_file(void)
{
	pthread_mutex_lock(&mutex);

	std::filesystem::path file_path = std::filesystem::path(store_parent) / DEV_INFO_FILE;
	std::ofstream fout(file_path);
	int count = 0;

	if (!fout.is_open()) {
		std::cerr << "Unable to open the file: " << file_path << std::endl;
		return -1;
	}

	for (const auto& info : devinfos) {
		if (!info.vaild)
			continue;
		fout << info.id << info.password << std::endl;
		count++;
	}

	fout.close();

	std::cout << "Saved: " << count << " devices" << std::endl;

	pthread_mutex_unlock(&mutex);

	return 0;
}

static struct device_info *app_get_dev_by_id(const std::string &id)
{
	struct device_info *retval = NULL;
	int count = (int)devinfos.size();

	for (int i = 0; i < count; i++) {
		if (devinfos[i].id == id) {
			retval = &devinfos[i];
			break;
		}
	}

	return retval;
}

static int app_pwd_sync_request(char *buffer, int buf_offs, int vaild_size, int buff_size)
{
	int retval = 0;
	int len;
	uint32_t count;
	uint32_t index;
	struct device_info *info_ptr;
	std::string dev_id;

	if (vaild_size < 1) {
		std::cerr << "Information command is incorrect" << std::endl;
		return -1;
	}

	pthread_mutex_lock(&mutex);

	if (buff_size - buf_offs < 2)
		goto nomem_err;

	switch (buffer[buf_offs]) {
	case SYNC_CMD_GET_COUNT:	/* Get the number of device information */
		if (buff_size - buf_offs < 1 + 1 + 4)
			goto nomem_err;
		count = devinfos.size();
		buffer[buf_offs + 1] = SYNC_RESULT_OK;
		buffer[buf_offs + 2] = (count >> 0) & 0xff;
		buffer[buf_offs + 3] = (count >> 8) & 0xff;
		buffer[buf_offs + 4] = (count >> 16) & 0xff;
		buffer[buf_offs + 5] = (count >> 24) & 0xff;
		retval = 6;
		break;
	case SYNC_CMD_GET_ITEM:		/* Get device information */
		if (vaild_size != 1 + 4)
			goto illegal_err;
		index = (buffer[buf_offs + 1] << 0) |
			(buffer[buf_offs + 2] << 8) |
			(buffer[buf_offs + 3] << 16) |
			(buffer[buf_offs + 4] << 24);
		len = 0;
		if (index >= devinfos.size()) {
			buffer[buf_offs + 1] = SYNC_RESULT_FAIL;
			std::cerr << "Get failed, index incorrect" << std::endl;
		} else {
			if (devinfos[index].vaild) {
				int id_len = devinfos[index].id.length();
				int pw_len = devinfos[index].password.length();
				if (buff_size - buf_offs < 2 + id_len + pw_len)
					goto nomem_err;
				memcpy(buffer + buf_offs + 2, devinfos[index].id.c_str(), id_len);
				memcpy(buffer + buf_offs + 2 + id_len,
				       devinfos[index].password.c_str(), pw_len);
				buffer[buf_offs + 1] = SYNC_RESULT_OK;
				len = id_len + pw_len;
			} else {
				buffer[buf_offs + 1] = SYNC_RESULT_DONE;
				std::cout << "Index invalid" << std::endl;
			}
		}
		retval = 2 + len;
		break;
	case SYNC_CMD_SET_DEV:		/* Set device information */
		if (vaild_size < 1 + DEV_ID_LEN)
			goto illegal_err;
		dev_id = std::string(buffer + buf_offs + 1, DEV_ID_LEN);
		info_ptr = app_get_dev_by_id(dev_id);
		if (!info_ptr) {
			buffer[buf_offs + 1] = SYNC_RESULT_FAIL;
			std::cerr << "Set failed, id incorrect" << std::endl;
		} else {
			if (info_ptr->vaild) {
				if (vaild_size - 1 - DEV_ID_LEN > DEV_PWD_MAX)
					goto illegal_err;
				info_ptr->password = std::string(buffer + buf_offs + 1 + DEV_ID_LEN);
				defer_save_count = DEFER_SAVE_MAX;
				buffer[buf_offs + 1] = SYNC_RESULT_OK;
				std::cerr << "Change: " << dev_id << std::endl;
			} else {
				buffer[buf_offs + 1] = SYNC_RESULT_DONE;
				std::cout << "Id invalid" << std::endl;
			}
		}
		retval = 2;
		break;
	case SYNC_CMD_ADD_DEV:		/* Add device information */
		len = vaild_size - 1;
		if (len > DEV_ID_LEN + DEV_PWD_MAX)
			goto illegal_err;
		if (len < DEV_ID_LEN)
			goto illegal_err;
		if (devinfos.size() >= DEV_NUM_MAX) {
			buffer[buf_offs + 1] = SYNC_RESULT_FAIL;
			std::cerr << "Add failed, Maximize the number" << std::endl;
		} else {
			std::string tmp(buffer + buf_offs + 1);
			device_info info;
			info.id = tmp.substr(0, DEV_ID_LEN);
			info.password = tmp.substr(DEV_ID_LEN);
			info.vaild = true;
			devinfos.push_back(info);
			defer_save_count = DEFER_SAVE_MAX;
			buffer[buf_offs + 1] = SYNC_RESULT_OK;
			std::cerr << "Add: " << info.id << std::endl;
		}
		retval = 2;
		break;
	case SYNC_CMD_REMOVE_DEV:	/* Remove device information */
		if (vaild_size < 1 + DEV_ID_LEN)
			goto illegal_err;
		dev_id = std::string(buffer + buf_offs + 1, DEV_ID_LEN);
		info_ptr = app_get_dev_by_id(dev_id);
		if (!info_ptr) {
			buffer[buf_offs + 1] = SYNC_RESULT_FAIL;
			std::cerr << "Remove failed, id incorrect" << std::endl;
		} else {
			if (info_ptr->vaild) {
				info_ptr->vaild = false;
				defer_save_count = DEFER_SAVE_MAX;
				buffer[buf_offs + 1] = SYNC_RESULT_OK;
				std::cerr << "Remove: " << dev_id << std::endl;
			} else {
				buffer[buf_offs + 1] = SYNC_RESULT_DONE;
				std::cout << "Id invalid" << std::endl;
			}
		}
		retval = 2;
		break;
	default:
		goto illegal_err;
	}

	pthread_mutex_unlock(&mutex);

	return retval;
nomem_err:
	std::cerr << "Not enough buffer space" << std::endl;
	pthread_mutex_unlock(&mutex);
	return -1;
illegal_err:
	std::cerr << "Lllegal command" << std::endl;
	pthread_mutex_unlock(&mutex);
	return -1;
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
	int last_count = 0;
	struct event_bus_msg msg = { EVENT_BUS_STARTUP, 0, 0};

	(void)argc;
	(void)argv;

	std::cout << "Hello Hoozz Play" << std::endl;

	store_init();
	store_parent = std::string(get_store_parent_path());
	std::cout << "store parent: " << store_parent << std::endl;

	/* Data loading */
	app_read_data_file();

	/* Event bus */
	event_bus_init();
	event_bus_register(app_event_notify_callback);
	event_bus_send(&msg);

	/* Network ctrl */
	simple_ctrl_init();
	simple_ctrl_set_name("PWD SYNC");
	simple_ctrl_set_class_id(CLASS_ID_PWD_SYNC);
	simple_ctrl_request_register(app_pwd_sync_request);

	/* Network ready */
	msg.type = EVENT_BUS_WIFI_CONNECTED;
	event_bus_send(&msg);

	while (1) {
		usleep(100000); // 100ms
		if (defer_save_count)
			defer_save_count--;
		if (!defer_save_count && last_count)
			app_write_data_file();
		last_count = defer_save_count;
	}

	return 0;
}
