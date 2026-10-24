#define W10L_ROAST_IMPLEMENTATION
#include "W10L-roaster.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#else
#include <curl/curl.h>
#include <unistd.h>
#endif

static void sleep_ms(unsigned ms)
{
    #ifdef _WIN32
    Sleep(ms);
    #else
    usleep(ms * 1000u);
    #endif
}

static int json_escape(char *dst, size_t n, const char *src)
{
    size_t j = 0;
    for (size_t i = 0; src[i]; i++) {
        unsigned char c = (unsigned char)src[i];
        const char *rep = NULL;
        char tmp[8];
        if (c == '"') rep = "\\\"";
        else if (c == '\\') rep = "\\\\";
        else if (c == '\n') rep = "\\n";
        else if (c == '\r') rep = "\\r";
        else if (c == '\t') rep = "\\t";
        else if (c < 0x20) { snprintf(tmp, sizeof tmp, "\\u%04x", c); rep = tmp; }
        if (rep) {
            size_t l = strlen(rep);
            if (j + l >= n) return -1;
            memcpy(dst + j, rep, l);
            j += l;
        } else {
            if (j + 1 >= n) return -1;
            dst[j++] = (char)c;
        }
    }
    dst[j] = '\0';
    return (int)j;
}

#ifdef _WIN32

static int post_webhook(const char *url, const char *json)
{
    wchar_t wurl[2048];
    wchar_t host[256];
    wchar_t path[2048];
    wchar_t extra[1024];
    URL_COMPONENTS uc;
    HINTERNET ses = NULL, con = NULL, req = NULL;
    DWORD status = 0, sz = sizeof status, len = (DWORD)strlen(json);
    int rc = -1;

    if (!MultiByteToWideChar(CP_UTF8, 0, url, -1, wurl, 2048)) return -1;

    memset(&uc, 0, sizeof uc);
    uc.dwStructSize = sizeof uc;
    uc.lpszHostName = host;
    uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 2048;
    uc.lpszExtraInfo = extra;
    uc.dwExtraInfoLength = 1024;
    if (!WinHttpCrackUrl(wurl, 0, 0, &uc)) return -1;
    if (wcslen(path) + wcslen(extra) + 1 > 2048) return -1;
    wcscat(path, extra);

    ses = WinHttpOpen(L"w10l-roaster/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                      WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!ses) goto done;
    con = WinHttpConnect(ses, host, uc.nPort, 0);
    if (!con) goto done;
    req = WinHttpOpenRequest(con, L"POST", path, NULL, WINHTTP_NO_REFERER,
                             WINHTTP_DEFAULT_ACCEPT_TYPES,
                             uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (!req) goto done;
    if (!WinHttpSendRequest(req, L"Content-Type: application/json", (DWORD)-1L,
        (LPVOID)json, len, len, 0)) goto done;
    if (!WinHttpReceiveResponse(req, NULL)) goto done;
    if (!WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz,
        WINHTTP_NO_HEADER_INDEX)) goto done;
    rc = (int)status;
    done:
    if (req) WinHttpCloseHandle(req);
    if (con) WinHttpCloseHandle(con);
    if (ses) WinHttpCloseHandle(ses);
    return rc;
}

#else

static size_t discard(void *p, size_t s, size_t n, void *u)
{
    (void)p; (void)u;
    return s * n;
}

static int post_webhook(const char *url, const char *json)
{
    CURL *c = curl_easy_init();
    struct curl_slist *h = NULL;
    long status = 0;
    int rc = -1;

    if (!c) return -1;
    h = curl_slist_append(h, "Content-Type: application/json");
    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_HTTPHEADER, h);
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(c, CURLOPT_USERAGENT, "w10l-roaster/1.0");
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, discard);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 15L);
    if (curl_easy_perform(c) == CURLE_OK) {
        curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &status);
        rc = (int)status;
    }
    curl_slist_free_all(h);
    curl_easy_cleanup(c);
    return rc;
}

#endif

static void usage(const char *p)
{
    fprintf(stderr,
            "usage: %s [-n name] [-l 0-2] [-c count] [-s seed] [--clean] [--webhook url]\n"
            "  no --webhook : press Enter for a roast, q + Enter to quit\n"
            "  --webhook    : send roast(s) to the webhook and exit (or set W10L_WEBHOOK)\n",
            p);
}

int main(int argc, char **argv)
{
    const char *name = "W10L";
    const char *webhook = getenv("W10L_WEBHOOK");
    int level = 1, profane = 1, count = 1;
    unsigned long long seed = 0;
    int use_webhook = 0;
    char roast[1024];
    char esc[2200];
    char body[2400];

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-n") && i + 1 < argc) name = argv[++i];
        else if (!strcmp(argv[i], "-l") && i + 1 < argc) level = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-c") && i + 1 < argc) count = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-s") && i + 1 < argc) seed = strtoull(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--clean")) profane = 0;
        else if (!strcmp(argv[i], "--webhook")) {
            use_webhook = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') webhook = argv[++i];
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (count < 1) count = 1;
    w10l_roast_seed(seed);

    if (use_webhook) {
        if (!webhook || !*webhook) {
            fprintf(stderr, "no webhook url given\n");
            return 1;
        }
        #ifndef _WIN32
        curl_global_init(CURL_GLOBAL_DEFAULT);
        #endif
        int fail = 0;
        for (int i = 0; i < count; i++) {
            if (w10l_roast(roast, sizeof roast, name, level, profane) < 0) { fail = 1; break; }
            if (json_escape(esc, sizeof esc, roast) < 0) { fail = 1; break; }
            snprintf(body, sizeof body, "{\"content\":\"%s\"}", esc);
            int st = post_webhook(webhook, body);
            if (st >= 200 && st < 300) printf("sent (%d): %s\n", st, roast);
            else { fprintf(stderr, "send failed (status %d)\n", st); fail = 1; break; }
            if (i + 1 < count) sleep_ms(1500);
        }
        #ifndef _WIN32
        curl_global_cleanup();
        #endif
        return fail;
    }

    char line[64];
    puts("Enter = roast, q = quit");
    for (;;) {
        if (!fgets(line, sizeof line, stdin)) break;
        if (line[0] == 'q' || line[0] == 'Q') break;
        if (w10l_roast(roast, sizeof roast, name, level, profane) < 0) return 1;
        puts(roast);
    }
    return 0;
}
