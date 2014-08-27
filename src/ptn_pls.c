
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
	return "http://prem1.di.fm:80/vocaltrance_hi?643945d6aa1da7d29705e61b";
}

char*
ptn_get_stream_url(char *url)
{
	CURL *ch;
	CURLcode result;
	char *station_url;
	char *pls_f_name = tmpnam(NULL);
	FILE *pls_f = fopen(pls_f_name, "wb+");

	if (!strstr(url, ".pls"))
		return url;

	ch = curl_easy_init();
	curl_easy_setopt(ch, CURLOPT_URL, url);
	curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void *) pls_f);
	result = curl_easy_perform(ch);

	if (result != CURLE_OK) {
		ptn_debug("Error loading .pls file: %s", curl_easy_strerror(result));
		return "\0";
	}

	curl_easy_cleanup(ch);
	station_url = ptn_parse_pls_file(pls_f);
	fclose(pls_f);
	remove(pls_f_name);

	return station_url;
}
