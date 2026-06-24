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

#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#define STORE_PATH		"data"

void store_init(void)
{
	struct stat st;

	if (stat(STORE_PATH, &st) == -1) {
		if (errno == ENOENT)
			mkdir(STORE_PATH, 0755);
	}
}

int store_save(char *name, char *data, int size)
{
	char path[32];
	FILE* f;
	int ret;

	sprintf(path, STORE_PATH "/%s", name);
	printf("Open: %s\n", path);

	f = fopen(path, "wb");
	if (f == NULL) {
		printf("Failed to open file for writing\n");
		return -1;
	}

	ret = fwrite(data, 1, size, f);
	if (ret != size)
		printf("File write failed, ret:%d\n", ret);
	else
		printf("File written\n");

	fclose(f);

	return 0;
}

int store_load(char *name, char *buff, int size)
{
	char path[32];
	FILE* f;
	int ret;

	sprintf(path, STORE_PATH "/%s", name);
	printf("Open %s\n", path);

	f = fopen(path, "rb");
	if (f == NULL) {
		printf("Failed to open file for reading\n");
		return -1;
	}

	ret = fread(buff, 1, size, f);
	fclose(f);

	if (ret <= 0) {
		printf("Reading error or file is empty\n");
		return -1;
	} else {
		printf("File read size: %d bytes\n", ret);
	}

	return 0;
}

void os_get_fixed_id(uint8_t *id)
{
	const char *filename = STORE_PATH "/id";
	struct stat st;
	FILE *fp;
	int i;

	if (stat(filename, &st)) {
		fp = fopen(filename, "wb");
		if (!fp) {
			perror("create failed");
			return;
		}
		
		srand(time(NULL));
		for (i = 0; i < 6; i++)
			id[i] = rand() % 256;
		
		fwrite(id, 1, 6, fp);
		fclose(fp);

		printf("A new ID has been created\n");
	} else {
		fp = fopen(filename, "rb");
		if (!fp) {
			perror("open failed");
			return;
		}
		
		fread(id, 1, 6, fp);
		fclose(fp);
	}
}

const char *get_store_parent_path(void)
{
	return STORE_PATH;
}
