#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>

#include "linenoise.h"

bool stopped = false;

void print_events(uint32_t events)
{
    if (events & EPOLLIN) printf(" EPOLLIN");
    if (events & EPOLLOUT) printf(" EPOLLOUT");
    if (events & EPOLLRDHUP) printf(" EPOLLRDHUP");
    if (events & EPOLLPRI) printf(" EPOLLPRI");
    if (events & EPOLLERR) printf(" EPOLLERR");
    if (events & EPOLLHUP) printf(" EPOLLHUP");
    if (events & EPOLLET) printf(" EPOLLET");
    if (events & EPOLLONESHOT) printf(" EPOLLONESHOT");
}

void callback(const char *line)
{
    printf("Line is '%s'\n", line);

    if (!strcmp(line, "quit")) stopped = true;
    /* check for input changing cmd */
    size_t len = strlen(line);
    if (len > 2 && line[0] == '[' && line[len -1] == ']') {
	char *buf = strdup(&line[1]), newPrompt[128];
	/* remove trailing garbage */
	buf[len-2] = '\0';

	/* modify readline's prompt */
	snprintf(newPrompt, sizeof(newPrompt), "{%s}(gdb)> ", buf);
	linenoiseSetPrompt(newPrompt);
	free(buf);
	return;
    }

    linenoiseHistoryAdd(line); /* Add to the history. */
}

int main(void)
{
    int epollFD;

    epollFD = epoll_create1(EPOLL_CLOEXEC);
    if (epollFD < 0) {
	printf("Failed to create epollFD: %m\n");
	exit(1);
    }

    struct epoll_event ev = {
	.events = EPOLLIN | EPOLLPRI,
	.data.fd = STDIN_FILENO };
    int ret = epoll_ctl(epollFD, EPOLL_CTL_ADD, ev.data.fd, &ev);
    if (ret < 0) {
	printf("epoll_ctl() failed: %m\n");
	exit(1);
    }

    linenoiseSetHandlerCallback(NULL, callback);
    linenoiseSetPrompt("input>");
    linenoiseForcedUpdateDisplay();

    while(!stopped) {
#define MAX_EVENTS 10
	struct epoll_event events[MAX_EVENTS];

	int nfds = epoll_wait(epollFD, events, MAX_EVENTS, -1);

	if (nfds < 0 ) {
	    printf("epoll_wait: %m\n");
	    if (errno == EINTR) continue;
	    return -1;
	}

	/* handle selected fds */
	for (int s=0; s<nfds; s++) {
	    int fd = events[s].data.fd;

	    if (fd < 0) continue;

	    if (events[s].events & EPOLLIN) {
		switch (fd) {
		case STDIN_FILENO:
		    linenoiseReadChar();
		    break;
		default:
		    printf("Data available on %d\n", fd);
		}
	    }
	    if (events[s].events & EPOLLHUP) {
		printf("HUP on sock %d\n", fd);
	    }
	    if (events[s].events & EPOLLERR) {
		printf("error happened on fd %d\n", fd);
		int r = epoll_ctl(epollFD, EPOLL_CTL_DEL, fd, NULL);
		printf("epoll_ctl(CTL_DEL) returns %d: %m\n", r);
		close(fd);
	    }
	}
    }

    sleep(1);

    linenoiseRemoveHandlerCallback();
    printf("\n");

    return 0;
}
