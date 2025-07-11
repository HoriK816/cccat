#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#define BUFSIZE 4096
#define MAX_LINE 4096
#define TERMINAL_WIDTH 80
#define LINE_DISPLAY_WIDTH 64

int opt_number_all = 0;
int opt_number_nonblank = 0;
int opt_show_ends = 0;
int opt_show_tabs = 0;
int opt_center_left = 0;
int opt_use_color = 0;

void print_with_color(char c) {
    if (isdigit(c)) {
        dprintf(1, "\033[1;34m%c\033[0m", c);
    } else if (isalpha(c)) {
        dprintf(1, "\033[1;32m%c\033[0m", c);
    } else {
        dprintf(1, "%c", c);
    }
}

void print_line(const char *linebuf, int line_num) {
    int content_width = 0;
    int line_len = strlen(linebuf);
    char expanded_line[MAX_LINE * 4];
    int idx = 0;

    // タブや^I，$などの展開処理
    for (int i = 0; i < line_len; i++) {
        if (opt_show_tabs && linebuf[i] == '\t') {
            strcpy(&expanded_line[idx], "^I");
            idx += 2;
            content_width += 2;
        } else {
            expanded_line[idx++] = linebuf[i];
            content_width++;
        }
    }

    if (opt_show_ends) {
        expanded_line[idx++] = '$';
        content_width++;
    }

    expanded_line[idx] = '\0';

    // 全体のセンタリング（64列の出力を80列の中央に配置）
    int margin = (TERMINAL_WIDTH - LINE_DISPLAY_WIDTH) / 2;
    for (int i = 0; i < margin; i++) {
        dprintf(1, " ");
    }

    // 左揃えで行番号表示
    if (opt_number_all || (opt_number_nonblank && linebuf[0] != '\n' && linebuf[0] != '\0')) {
        dprintf(1, "%6d\t", line_num);
    }

    // 本文表示（色付きオプションあり）
    for (int i = 0; i < idx; i++) {
        if (opt_use_color) {
            print_with_color(expanded_line[i]);
        } else {
            write(1, &expanded_line[i], 1);
        }
    }

    dprintf(1, "\n");
}

int enhanced_cat(int fd, const char *filename) {
    char buf[BUFSIZE];
    char linebuf[MAX_LINE];
    ssize_t n;
    int i, line_idx = 0, line_num = 1;

    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        for (i = 0; i < n; i++) {
            char c = buf[i];
            if (line_idx < MAX_LINE - 1) {
                linebuf[line_idx++] = c;
            }

            if (c == '\n') {
                linebuf[line_idx - 1] = '\0';  // 改行を除去
                print_line(linebuf, line_num);
                if (opt_number_all || (opt_number_nonblank && linebuf[0] != '\0')) {
                    line_num++;
                }
                line_idx = 0;
            }
        }
    }

    // 改行で終わってない最終行
    if (line_idx > 0) {
        linebuf[line_idx] = '\0';
        print_line(linebuf, line_num);
    }

    if (n < 0) {
        perror(filename);
        return 1;
    }
    return 0;
}

void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [options] [file...]\n"
        "Options:\n"
        "  -n   Number all lines\n"
        "  -b   Number non-empty lines only\n"
        "  -E   Show $ at end of lines\n"
        "  -T   Show TAB characters as ^I\n"
        "  -c   Center the entire line output\n"
        "  -C   Color output by character type\n",
        prog
    );
}

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "nbETcC")) != -1) {
        switch (opt) {
            case 'n': opt_number_all = 1; break;
            case 'b': opt_number_nonblank = 1; break;
            case 'E': opt_show_ends = 1; break;
            case 'T': opt_show_tabs = 1; break;
            case 'c': opt_center_left = 1; break;
            case 'C': opt_use_color = 1; break;
            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (opt_number_all && opt_number_nonblank) {
        fprintf(stderr, "Error: -n and -b are mutually exclusive\n");
        exit(EXIT_FAILURE);
    }

    if (optind == argc) {
        // 標準入力
        enhanced_cat(STDIN_FILENO, "stdin");
    } else {
        for (int i = optind; i < argc; i++) {
            int fd = open(argv[i], O_RDONLY);
            if (fd < 0) {
                perror(argv[i]);
                continue;
            }
            enhanced_cat(fd, argv[i]);
            close(fd);
        }
    }

    return 0;
}

