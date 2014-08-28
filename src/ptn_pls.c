
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
	if (!fseek(f, 11, SEEK_SET)) {
		ptn_debug(".pls malformed");
		return NULL;
	}

	while (fscanf(f, "%19s=%199[^n]", key, value) == 2) {
		if (strstr(key, "File"))
			return strdup(value);
	}

	return NULL;
}

char*
ptn_get_stream_url(char *url)
{
	CURL *ch;
	CURLcode result;
	char *station_url;
	char *pls_f_name = tmpnam(NULL);
	FILE *pls_f = fopen(pls_f_name, "w+");
	printf("%s", pls_f_name);

	if (!strstr(url, ".pls"))
		return strdup(url);

	ch = curl_easy_init();
	curl_easy_setopt(ch, CURLOPT_URL, url);
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void *) pls_f);
	result = curl_easy_perform(ch);

	if (result != CURLE_OK) {
		ptn_debug("Error loading .pls file: %s", curl_easy_strerror(result));
		return NULL;
	}

	curl_easy_cleanup(ch);
	station_url = ptn_parse_pls_file(pls_f);
	fclose(pls_f);
	remove(pls_f_name);

	return station_url;
}
