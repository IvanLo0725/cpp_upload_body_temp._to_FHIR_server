/*
 * fhir_upload_temperature.c
 *
 * Simple C program to read a body temperature from stdin/argument and upload
 * it as an Observation resource (FHIR R4) to a FHIR server (HAPI FHIR public instance by default).
 *
 * Prerequisites:
 *   - libcurl (development headers) installed.
 *   - Compile: gcc -o fhir_upload fhir_upload_temperature.c -lcurl -lm
 *
 * Usage:
 *   ./fhir_upload 37.2
 *   or
 *   ./fhir_upload         (then input temperature when prompted)
 *
 * The program posts to: https://hapi.fhir.org/baseR4/Observation
 * Patient ID is set to: 49410276 by default (you can change PATIENT_ID macro below).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <curl/curl.h>

#define FHIR_SERVER_URL "https://hapi.fhir.org/baseR4/Observation"
#define PATIENT_ID "49410276"

/* Helper: produce current UTC time in ISO 8601 format (YYYY-MM-DDTHH:MM:SSZ) */
static void current_time_iso8601(char *buf, size_t bufsz) {
    time_t t = time(NULL);
    struct tm tm;
#if defined(_WIN32) || defined(_WIN64)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    strftime(buf, bufsz, "%Y-%m-%dT%H:%M:%SZ", &tm);
}

int main(int argc, char **argv) {
    double temp_value = NAN;

    if (argc >= 2) {
        temp_value = atof(argv[1]);
    } else {
        char input[64];
        printf("Enter body temperature (e.g. 36.5): ");
        if (!fgets(input, sizeof(input), stdin)) {
            fprintf(stderr, "No input\n");
            return 1;
        }
        temp_value = atof(input);
    }

    if (!isfinite(temp_value)) {
        fprintf(stderr, "Invalid temperature value.\n");
        return 1;
    }

    /* Prepare Observation JSON per FHIR R4 */
    char iso_time[32];
    current_time_iso8601(iso_time, sizeof(iso_time));

    /* Build JSON payload. Keep buffer large enough for safe formatting. */
    char json[2048];
    int n = snprintf(json, sizeof(json),
        "{\n"
        "  \"resourceType\": \"Observation\",\n"
        "  \"status\": \"final\",\n"
        "  \"category\": [ { \"coding\": [ { \"system\": \"http://terminology.hl7.org/CodeSystem/observation-category\", \"code\": \"vital-signs\", \"display\": \"Vital Signs\" } ] } ],\n"
        "  \"code\": { \"coding\": [ { \"system\": \"http://loinc.org\", \"code\": \"8310-5\", \"display\": \"Body temperature\" } ], \"text\": \"Body temperature\" },\n"
        "  \"subject\": { \"reference\": \"Patient/%s\" },\n"
        "  \"effectiveDateTime\": \"%s\",\n"
        "  \"valueQuantity\": { \"value\": %.2f, \"unit\": \"degrees C\", \"system\": \"http://unitsofmeasure.org\", \"code\": \"Cel\" }\n"
        "}\n",
        PATIENT_ID, iso_time, temp_value);

    if (n < 0 || n >= (int)sizeof(json)) {
        fprintf(stderr, "JSON payload too long or formatting error.\n");
        return 1;
    }

    /* Initialize libcurl and perform HTTP POST */
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl.\n");
        return 1;
    }

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/fhir+json;charset=utf-8");
    headers = curl_slist_append(headers, "Accept: application/fhir+json");

    CURLcode res;
    curl_easy_setopt(curl, CURLOPT_URL, FHIR_SERVER_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(json));
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "fhir-c-uploader/1.0");
    curl_easy_setopt(curl, CURLOPT_CAINFO, "curl-ca-bundle.crt");

    /* Optionally: enable verbose debug output by uncommenting the next line */
    /* curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); */

    printf("Posting Observation to %s\n", FHIR_SERVER_URL);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return 1;
    }

    /* Print HTTP response code and any response body */
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    printf("Server HTTP response code: %ld\n", http_code);

    /* Clean up */
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (http_code >= 200 && http_code < 300) {
        printf("Observation uploaded successfully.\n");
        return 0;
    } else {
        printf("Upload may have failed. Check server logs or response.\n");
        return 1;
    }
}


