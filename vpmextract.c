// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2024 Ernesto A. Fernández <ernesto.mnd.fernandez@gmail.com>
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char *progname = NULL;

static void usage(void)
{
	fprintf(stderr, "usage: %s file.vpm target-dir\n", progname);
	exit(1);
}

struct vpm_header {
	char		vpm_magic[6];
	char		vpm_version[3];
	uint8_t		vpm_second;
	uint8_t		vpm_minute;
	uint8_t		vpm_hour;
	uint8_t		vpm_day;
	uint8_t		vpm_month;
	uint16_t	vpm_year;
	uint16_t	vpm_list_offset;
	uint8_t		vpm_lang_id;
};

struct vpm_list_entry {
	uint32_t	vpm_wav_offset;
	uint32_t	vpm_wav_length;
};

static char *read_whole_vpm(const char *srcname, unsigned long *size_p)
{
	FILE *file = NULL;
	char *buf = NULL;
	long size;

	file = fopen(srcname, "rb");
	if (!file) {
		perror(progname);
		exit(1);
	}
	if (fseek(file, 0, SEEK_END)) {
		perror(progname);
		exit(1);
	}
	size = ftell(file);
	if (size < 0) {
		perror(progname);
		exit(1);
	}
	if (size > 50 * 1024 * 1024) {
		fprintf(stderr, "%s: file is huge (%lu), is it really a vpm?\n", progname, size);
		exit(1);
	}
	buf = malloc(size);
	if (!buf) {
		perror(progname);
		exit(1);
	}
	if (fseek(file, 0, SEEK_SET)) {
		perror(progname);
		exit(1);
	}
	if (fread(buf, size, 1, file) != 1) {
		fprintf(stderr, "%s: file read failed for %s\n", progname, srcname);
		exit(1);
	}
	*size_p = size;
	return buf;
}

static char *make_target_path(const char *dirname, int number)
{
	char *path = NULL;
	size_t pathlen;

	pathlen = strlen(dirname) + 128; /* No need to be precise */
	path = malloc(pathlen);
	if (!path) {
		perror(progname);
		exit(1);
	}
	snprintf(path, pathlen, "%s/%.4u.wav", dirname, number);
	return path;
}

static void extract_wav_file(const char *dirname, const char *vpm, unsigned long offset, unsigned long length, int idx)
{
	char *target_path = NULL;
	FILE *file = NULL;

	target_path = make_target_path(dirname, idx);
	file = fopen(target_path, "wb");
	if (!file) {
		perror(progname);
		exit(1);
	}
	if (fwrite(vpm + offset, 1, length, file) != length) {
		fprintf(stderr, "%s: file write failed for %s\n", progname, target_path);
		exit(1);
	}
	if (fclose(file)) {
		perror(progname);
		exit(1);
	}
	free(target_path);
}

static void extract_whole_vpm(const char *srcname, const char *dirname)
{
	struct vpm_header *hdr = NULL;
	struct vpm_list_entry *entry = NULL, *first_entry = NULL;
	char *vpm = NULL;
	unsigned long size, off, wav_off, wav_len;
	int idx;

	vpm = read_whole_vpm(srcname, &size);
	if (size < sizeof(*hdr)) {
		fprintf(stderr, "%s: source file is too small\n", progname);
		exit(1);
	}
	hdr = (struct vpm_header *)vpm;
	if (memcmp(hdr->vpm_magic, "AUDIMG", sizeof(hdr->vpm_magic)) != 0) {
		fprintf(stderr, "%s: not a Garmin vpm file (wrong magic)\n", progname);
		exit(1);
	}

	first_entry = (struct vpm_list_entry *)(vpm + hdr->vpm_list_offset);
	for (idx = 0;; ++idx) {
		off = hdr->vpm_list_offset + idx * sizeof(*entry);
		if (off + sizeof(*entry) < off || off + sizeof(*entry) > size) {
			fprintf(stderr, "%s: offset array is too big for the file\n", progname);
			exit(1);
		}
		/* Hacky, I don't know how to tell the size of the array */
		if (off >= first_entry->vpm_wav_offset)
			break;
		entry = (struct vpm_list_entry *)(vpm + off);

		wav_off = entry->vpm_wav_offset;
		wav_len = entry->vpm_wav_length;
		if (wav_off + wav_len < wav_off || wav_off + wav_len > size) {
			fprintf(stderr, "%s: wav file is out of bounds\n", progname);
			exit(1);
		}
		extract_wav_file(dirname, vpm, wav_off, wav_len, idx);
	}

	free(vpm);
}

int main(int argc, char *argv[])
{
	char *srcname = NULL;
	char *dirname = NULL;

	if (argc == 0)
		exit(1);
	progname = argv[0];
	if (argc != 3)
		usage();
	srcname = argv[1];
	dirname = argv[2];

	extract_whole_vpm(srcname, dirname);
}
