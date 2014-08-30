
/*
 * Copyright (C) Jacob Budin
 */

#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include "pituner.h"

char*
ptn_parse_pls_file(FILE *f)
{
	char key[20], value[200];

	// skip "[playlist]" first line
	if (fseek(f, 11, SEEK_SET) != 0) {
		ptn_debug(".pls malformed");
		return NULL;
	}

	while (fscanf(f, "%19[^=]=%199[^\n]", key, value) == 2) {
		if (strstr(key, "File"))
			return strdup(value);
	}

	ptn_debug(".pls does not contain File key");

	return NULL;
}

char*
ptn_get_stream_url(char *url)
{
	CURL *ch;
	CURLcode result;
	char *station_url;
	char *pls_f_name;
	FILE *pls_f;

	pls_f_name = tmpnam(NULL);

	if (!pls_f_name)
		ptn_error("Cannot create temporary .pls file name");

	pls_f = fopen(pls_f_name, "w+");

	if (!pls_f)
		ptn_error("Cannot create temporary .pls file");

	if (!strstr(url, ".pls"))
		return strdup(url);

	ch = curl_easy_init();

	if (!ch)
		ptn_error("Cannot intiate curl session");

	curl_easy_setopt(ch, CURLOPT_URL, url);
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void *) pls_f);
	result = curl_easy_perform(ch);

	if (result != CURLE_OK) {
		ptn_debug("Error loading .pls file: %s", curl_easy_strerror(result));
		return NULL;
	}

	curl_easy_cleanup(ch);
	rewind(pls_f);
	station_url = ptn_parse_pls_file(pls_f);
	fclose(pls_f);
	remove(pls_f_name);

	return station_url;
}
