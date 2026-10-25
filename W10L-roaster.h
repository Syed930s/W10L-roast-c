
#ifndef W10L_ROAST_H
#define W10L_ROAST_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
    #endif

    void        w10l_roast_seed(unsigned long long seed);
    int         w10l_roast(char *buf, size_t n, const char *target, int level, int profane);
    const char *w10l_roast_version(void);

    #ifdef __cplusplus
}
#endif

#endif /* W10L_ROAST_H */

#if defined(W10L_ROAST_IMPLEMENTATION) || defined(W10L_ROAST_DEMO)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define W10L_ARRLEN(a) (sizeof(a) / sizeof((a)[0]))

/* ---- tiny xorshift64* PRNG ---- */
static unsigned long long w10l__state = 0x9E3779B97F4A7C15ULL;

static unsigned long long w10l__next(void)
{
    unsigned long long x = w10l__state;
    x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
    w10l__state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

static size_t w10l__pick(size_t n) { return (size_t)(w10l__next() % n); }

void w10l_roast_seed(unsigned long long seed)
{
    if (!seed) seed = (unsigned long long)time(NULL) ^ ((unsigned long long)clock() << 32);
    w10l__state = seed ? seed : 0x9E3779B97F4A7C15ULL;
    for (int i = 0; i < 8; i++) (void)w10l__next();
}

const char *w10l_roast_version(void) { return "w10l-roast-c 1.0.0"; }

/* ---- content pools. Every %s is the target's name. ---- */

/* openers */
static const char *w10l__open_clean[] = {
    "Listen up, %s:", "Breaking news, %s:", "Hey %s,", "Quick reminder, %s:",
    "Bad news, %s:", "Look, %s,", "Public service announcement for %s:"
};
static const char *w10l__open_swear[] = {
    "Listen up, you absolute clown %s:", "Hey %s, dumbass:", "Yo %s, shut the hell up for a sec:",
    "Bad news, %s, you walnut-brained goblin:", "Oi %s, pay attention, dipshit:"
};

/* mild cores */
static const char *w10l__core_mild[] = {
    "you defend an OS that hit end-of-life and you're still acting like it's the future.",
    "your Windows 10 install has more unpatched holes than your arguments.",
    "even Clippy would uninstall himself after seeing your takes.",
    "you're the only person who treats Patch Tuesday like a personality trait.",
    "your hot takes are slower than Windows Update on a spinning HDD.",
    "you bluescreen harder than your own PC when you open your mouth.",
    "your whole identity is a version number Microsoft already abandoned.",
    "the only thing you've ever optimised is how fast you lose arguments.",
    "your tech knowledge peaked at 'have you tried restarting it'.",
    "you're running on legacy hardware, legacy software and legacy brain cells.",
    "your uptime is impressive and so is your complete lack of updates.",
    "you treat 'it works on my machine' like a legal defence.",
    "the only thing you've ever installed successfully is your own ego.",
    "you read 'End of Support' and somehow took it as a personal challenge."
};

/* spicy cores (clean but sharper) */
static const char *w10l__core_spicy[] = {
    "you'd lose a debate to a Notepad window, and Notepad has autosave now.",
    "the wintuber community saw you coming and still hit the 'block' button faster than Task Manager kills a frozen app.",
    "you built a reputation so bad that even the Recycle Bin rejects it.",
    "every time you post, a Windows 10 box somewhere kernel panics out of secondhand embarrassment.",
    "you've got the confidence of a Linux user and the skills of a Windows ME speedrunner.",
    "nobody asked for your opinion, and the telemetry agrees.",
    "you're proof that 'Run as administrator' doesn't make you one.",
    "you've been thoroughly benchmarked and the results are 'deprecated'.",
    "you argue like a driver written in 2009: outdated, unstable and nobody wants to touch you.",
    "your reputation has more red flags than a Defender scan of a keygen folder.",
    "you've been banned from so many servers that Discord should issue you a loyalty card.",
    "every update you skip is one more reason the whole scene stopped listening."
};

/* nuclear cores (clean-ish, devastating) */
static const char *w10l__core_nuke[] = {
    "if cringe were a driver, you'd be the one that wouldn't sign.",
    "your credibility has been permanently deleted and it's not even in the Recycle Bin.",
    "you're the human equivalent of a 'Your device ran into a problem' screen.",
    "the community didn't cancel you, it just ran a disk cleanup and you were the temp files.",
    "you hold the world record for fastest speedrun from 'hot take' to 'ratio'.",
    "you're the reason Microsoft has a 'Sorry, something went wrong' dialog.",
    "you're a memory leak in the community and everyone's been waiting for the process to end.",
    "you ran 'sfc /scannow' on your personality and it found corruption it couldn't repair.",
    "your takes have been rolled back so many times they're basically in safe mode."
};

/* profane cores */
static const char *w10l__core_swear[] = {
    "you're a goddamn Windows 10 fanboy running an EOL piece of shit and calling it a lifestyle.",
    "your takes are fucking trash and your OS is just as dead as your reputation.",
    "you're so full of shit the Disk Cleanup tool would need a day and a half.",
    "nobody gives a damn about your opinions, you absolute bellend.",
    "you've got the technical skill of a bricked toaster and the personality to match.",
    "you're a walking dumpster fire with a Start menu.",
    "you fuck up so consistently that Microsoft should list you as a known issue.",
    "you're such a clown that the whole wintuber scene already put you on ignore, you pathetic twat.",
    "go touch grass, you shitty little noob, before you crash another conversation.",
    "your brain runs on a single-core Celeron and even that thread is deadlocked.",
    "you're a dead-OS-defending, update-dodging, shit-posting waste of bandwidth.",
    "you've got the charisma of a CMD window and none of the functionality, you useless prick.",
    "you're so fucking outdated even the Wayback Machine skips your pages.",
    "shut the fuck up and go patch your vulnerabilities, starting with the ones between your ears."
};

/* closers */
static const char *w10l__close_clean[] = {
    "Anyway, upgrade your brain.", "Try again after a reboot.", "Skill issue.",
    "Cope and update.", "Ctrl+Alt+Delete yourself out of this chat.",
    "Respectfully: log off.", "Have a nice day. Or don't. Telemetry is off."
};
static const char *w10l__close_swear[] = {
    "Now fuck off and update your shit.", "Skill issue, dumbass.", "Cry about it.",
    "Ctrl+Alt+Delete your whole damn account.", "Go reinstall your personality, clown.",
    "Get fucked, noob.", "Ratio'd and deprecated, loser."
};

/* ---- generator ---- */

static int w10l__append(char *buf, size_t n, size_t *len, const char *fmt, const char *name)
{
    if (*len >= n) return -1;
    int w = snprintf(buf + *len, n - *len, fmt, name);
    if (w < 0) return -1;
    if ((size_t)w >= n - *len) { *len = n - 1; return 0; }
    *len += (size_t)w;
    return 0;
}

static int w10l__lit(char *buf, size_t n, size_t *len, const char *s)
{
    if (*len >= n) return -1;
    int w = snprintf(buf + *len, n - *len, "%s", s);
    if (w < 0) return -1;
    if ((size_t)w >= n - *len) { *len = n - 1; return 0; }
    *len += (size_t)w;
    return 0;
}

int w10l_roast(char *buf, size_t n, const char *target, int level, int profane)
{
    if (!buf || n < 2) return -1;
    if (!target || !*target) target = "W10L";
    if (level < 0) level = 0;
    if (level > 2) level = 2;

    size_t len = 0;
    buf[0] = '\0';

    /* opener */
    if (profane) w10l__append(buf, n, &len, w10l__open_swear[w10l__pick(W10L_ARRLEN(w10l__open_swear))], target);
    else         w10l__append(buf, n, &len, w10l__open_clean[w10l__pick(W10L_ARRLEN(w10l__open_clean))], target);
    w10l__lit(buf, n, &len, " ");

    /* core(s): more lines at higher levels */
    int lines = 1 + level;
    for (int i = 0; i < lines; i++) {
        const char *c;
        if (profane && (level > 0 || (w10l__next() & 1)))
            c = w10l__core_swear[w10l__pick(W10L_ARRLEN(w10l__core_swear))];
        else if (level == 2 && i > 0)
            c = w10l__core_nuke[w10l__pick(W10L_ARRLEN(w10l__core_nuke))];
        else if (level >= 1 && i > 0)
            c = w10l__core_spicy[w10l__pick(W10L_ARRLEN(w10l__core_spicy))];
        else
            c = w10l__core_mild[w10l__pick(W10L_ARRLEN(w10l__core_mild))];
        w10l__lit(buf, n, &len, c);
        w10l__lit(buf, n, &len, " ");
    }

    /* closer */
    if (profane) w10l__lit(buf, n, &len, w10l__close_swear[w10l__pick(W10L_ARRLEN(w10l__close_swear))]);
    else         w10l__lit(buf, n, &len, w10l__close_clean[w10l__pick(W10L_ARRLEN(w10l__close_clean))]);

    return (int)len;
}

#endif /* implementation */

/* ---- demo CLI ---- */
#ifdef W10L_ROAST_DEMO
int main(int argc, char **argv)
{
    const char *name = "W10L";
    int level = 1, profane = 1, count = 1;
    unsigned long long seed = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-n") && i + 1 < argc)      name = argv[++i];
        else if (!strcmp(argv[i], "-l") && i + 1 < argc) level = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-c") && i + 1 < argc) count = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-s") && i + 1 < argc) seed = strtoull(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--clean"))            profane = 0;
        else if (!strcmp(argv[i], "--profane"))          profane = 1;
        else {
            fprintf(stderr, "usage: %s [-n name] [-l 0-2] [-c count] [-s seed] [--clean|--profane]\n", argv[0]);
            return 1;
        }
    }

    w10l_roast_seed(seed);
    char out[1024];
    for (int i = 0; i < count; i++) {
        if (w10l_roast(out, sizeof out, name, level, profane) < 0) return 1;
        puts(out);
    }
    return 0;
}
#endif
