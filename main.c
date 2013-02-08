/*
 * main.c
 *
 *  Created on: 02.02.2012
 *      Author: frank
 */

#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define BUFSIZE 256
#define FLAG_CANONICAL 	1<<0
#define FLAG_ECHO 		1<<0

struct termios termios_tty_old = {0};
struct termios termios_stdin_old = {0};
int tty;

void cleanup(int);

void usage(int argc, char** argv) {
	printf("Usage: %s [-ce] <tty> <config>\n", argv[0]);
	printf(" <tty> is the console to use (e.g. /dev/ttyS0).\n");
	printf(" <config> is a configuration specifier setting baud rate, parity bits, bit count, stop bit count and flow control.\n");
	printf("  eg. 9600n81h (9600 baud, no parity, 8 data bits, 1 stop bit, hw flow control)\n");
	printf("         ||||`-flow control (h - hardware cts/rts, s - software xon/xoff, omit for no flow control)\n");
	printf("         |||`--stop bits    (1 or 2)\n");
	printf("         ||`---data bits    (5, 6, 7 or 8)\n");
	printf("         |`----parity bit   (n - no parity, e - even parity, o - odd parity)\n");
	printf("         `-----baud rate    (50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800,\n");
	printf("                             2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400)\n");
	printf(" -e\techo all input from stdin to stdout\n");
	printf(" -c\tset stdin to canonical mode (output text lines rather than single characters)\n");
}

// i.e. 9600n81h
int decode_config(char* config, struct termios* termios) {
	char speedbuf[8] = {0};
	int speed = 0;
	int l = 0;
	speed_t baudrate = B0;
	// baud rate
	while(*config != '\0' && isdigit(*config) && l < 7) {
		speedbuf[l++] = *config;
		config++;
	}
	if(*config == '\0') return -1;
	speed = atoi(speedbuf);
#define MAP_SPEED(s) case s: baudrate = B ## s; break;
	switch(speed) {
		MAP_SPEED(50)
		MAP_SPEED(75)
		MAP_SPEED(110)
		MAP_SPEED(134)
		MAP_SPEED(150)
		MAP_SPEED(200)
		MAP_SPEED(300)
		MAP_SPEED(600)
		MAP_SPEED(1200)
		MAP_SPEED(1800)
		MAP_SPEED(2400)
		MAP_SPEED(4800)
		MAP_SPEED(9600)
		MAP_SPEED(19200)
		MAP_SPEED(38400)
		MAP_SPEED(57600)
		MAP_SPEED(115200)
		MAP_SPEED(230400)
		default:
			fprintf(stderr, "baud rate %d not supported\n", speed);
			return -1;
	}
#undef MAP_SPEED
	cfsetispeed(termios, baudrate);
	cfsetospeed(termios, baudrate);
	// parity
	char parity = *config++;
	switch(parity) {
		case 'o': termios->c_iflag |= INPCK; termios->c_cflag |= PARENB | PARODD; break;
		case 'e': termios->c_iflag |= INPCK; termios->c_cflag |= PARENB; break;
		case 'n': break;
		default:
			fprintf(stderr, "parity specification %c not known\n", parity);
			return -1;
	}
	if(*config == '\0') return -1;
	// bit count
	char bits = *config++;
	switch(bits) {
		case '5': termios->c_cflag |= CS5; break;
		case '6': termios->c_cflag |= CS6; break;
		case '7': termios->c_cflag |= CS7; break;
		case '8': termios->c_cflag |= CS8; break;
		default:
			fprintf(stderr, "bit count %c not supported\n", bits);
			return -1;
	}
	if(*config == '\0') return -1;
	// stop bit count
	char stopbits = *config++;
	switch(stopbits) {
		case '1': break;
		case '2': termios->c_cflag |= CSTOPB; break;
		default:
			fprintf(stderr, "stop bit count %c not supported\n", stopbits);
			return -1;
	}
	if(*config == '\0') return 0;
	// flow control
	char flow = *config;
	switch(flow) {
		case 'h': termios->c_cflag |= CRTSCTS; break;
		case 's': termios->c_iflag |= IXOFF; break;
	}
	return 0;
}

// parse cmd line options, return index of tty-filename
int parse_arg(int argc, char** argv, int* flags) {
	char* fmt = "ec";
	int opt;
	while((opt = getopt(argc, argv, fmt)) != -1) {
		switch(opt) {
			case 'e':
				*flags |= FLAG_ECHO;
				break;
			case 'c':
				*flags |= FLAG_CANONICAL;
				break;
			default:
				fprintf(stderr, "unknown option %c\n", opt);
				return -1;
		}
	}
	return optind;
}

// configure tty
void configure_tty(char* config, int flags) {
	struct termios termios_new = {0};
	speed_t baudrate = B0;
	tcgetattr(tty, &termios_tty_old); // safe current config
	if(decode_config(config, &termios_new) < 0) {
		fprintf(stderr, "cannot decode %s", config);
		exit(EXIT_FAILURE);
	}
	// enable read and ignore modem control lines
	termios_new.c_cflag |= CLOCAL | CREAD;
	tcsetattr(tty, TCSANOW, &termios_new);
}

// restore old tty config
void restore_tty() {
	tcsetattr(tty, TCSANOW, &termios_tty_old);
}

// configure stdin (noncanonical mode etc.)
void configure_stdin(int flags) {
	tcgetattr(STDIN_FILENO, &termios_stdin_old); // safe current config
	if((flags & FLAG_CANONICAL) && (flags & FLAG_ECHO)) return;
	struct termios termios_new = {0};
	memcpy(&termios_new, &termios_stdin_old, sizeof(struct termios));
	// if not otherwise specified, set to canonical mode (ie read single characters instead of lines)
	if(!(flags & FLAG_CANONICAL)) termios_new.c_lflag &= ~ICANON;
	// settle if characters should be echoed
	if(!(flags & FLAG_ECHO)) termios_new.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &termios_new);
}

// restore stdio config
void restore_stdin() {
	tcsetattr(STDIN_FILENO, TCSANOW, &termios_stdin_old);
}

void cleanup(int blub) {
	restore_stdin();
	restore_tty();
	close(tty);
	exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {
	int flags = 0;
	int filearg = 0;
	int stdin_hup = 0;
	filearg = parse_arg(argc, argv, &flags);
	if(filearg+1 >= argc || filearg < 0) {
		usage(argc, argv);
		exit(EXIT_FAILURE);
	}
	signal(SIGTERM, cleanup);
	signal(SIGINT, cleanup);
	// open specified terminal device
	tty = open(argv[filearg], O_RDWR | O_NOCTTY);
	if(tty < 0) {
		fprintf(stderr, "could not open '%s'", argv[1]);
		perror("");
		exit(EXIT_FAILURE);
	}
	// configure terminal device and stdin
	configure_tty(argv[filearg+1], flags);
	configure_stdin(flags);
	// do work (transfer things from one fd to the other)
	int nfds = 2;
	struct pollfd fds[2] = {{tty, POLLIN, 0}, {STDIN_FILENO, POLLIN, 0}};
	char buf_in[BUFSIZE+1]; // keep space for zero byte
	char buf_out[BUFSIZE+1];
	int bytes_sent = 0;
	int bytes_to_send = 0;
	while(1) {
		int r = poll(fds, nfds, -1);
		if(r < 0) {
			if(errno != EINTR) perror("error while polling tty and stdin");
			break;
		}
		if(fds[0].revents > 0) {
			// on the tty, consider both POLLIN and POLLOUT, since the write buffer can block easily
			if(fds[0].revents & (POLLERR | POLLHUP)) {
				break;
			}
			if(fds[0].revents & POLLIN) {
				int len = read(tty, buf_in, BUFSIZE);
				if(len < 0) {
					perror("ERROR: read() on tty");
					break;
				}
				len = write(STDOUT_FILENO, buf_in, len);
				if(len < 0) {
					perror("ERROR: write() on stdout");
					break;
				}
			}
			if(fds[0].revents & POLLOUT) {
				// send all bytes to tty
				assert(bytes_sent < BUFSIZE);
				int len = write(tty, buf_out+bytes_sent, bytes_to_send);
				if(len < 0) {
					if(errno == EAGAIN)
						continue;
					perror("ERROR: write() on tty");
					break;
				}
				bytes_sent += len;
				bytes_to_send -= len;
				assert(bytes_to_send >= 0);
				if(bytes_to_send == 0) {
					// all bytes in buffer were sent, buffer can be refilled
					fds[0].events = POLLIN;
				}
			}
		}
		if(fds[1].revents > 0 && !stdin_hup) {
			if(fds[1].revents & (POLLERR | POLLHUP)) {
				break;
			}
			if(fds[1].revents & POLLIN) {
				errno = 0;
				int len = read(STDIN_FILENO, buf_out, BUFSIZE);
				if(len == 0) {
					// stdin is at eof (eg. a file was redirected to stdin), close program
					stdin_hup = 1;
					nfds--;
				}
				if(len < 0) {
					perror("ERROR: read() on stdin");
					break;
				}
				// we have read some bytes, so next try to send them
				fds[0].events = POLLOUT;
				bytes_sent = 0;
				bytes_to_send = len;
				continue;
			}
		}
	}
	cleanup(0);
	return EXIT_FAILURE;
}
