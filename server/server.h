#ifndef SERVER_H
#define SERVER_H

/* Set the handler method for sig */
void addsig(int sig, void(handler)(int), bool is_restart);


int zr_server(const char *ip, const char *s_port);

#endif
