/*
 * Copyright 2016
 */

#include "config.h"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <jansson.h>
#include <string.h>
#include <unistd.h>

#include "ckpool.h"
#include "libckpool.h"
#include "rootstock.h"
#include "jansson_private.h"


static void send_rootstock(const ckpool_t *ckp, const char *msg)
{
	send_proc(ckp->rootstock, msg);
}

static const char *rsk_getwork_req = "{\"jsonrpc\": \"2.0\", \"method\": \"eth_getWork\", \"params\": [], \"id\": %d}\n";

bool rsk_getwork(connsock_t *cs, rsk_getwork_t *rgw)
{
	ckpool_t *ckp = cs->ckp;
	rdata_t *rdata = ckp->rdata;
	json_t *res_val, *val;
	bool ret = false;
	char *rpc_req;
	size_t len = 67 + 16; // strlen(rsk_getwork_req) + len(id)
	const char* blockhashmerge;
	const char* difficulty;
	char tmp[32], diff_swap[32];
	int id;

	id = ++rdata->lastreqid;
	rpc_req = ckalloc(len);
	sprintf(rpc_req, rsk_getwork_req, id);

	val = json_rpc_call(cs, rpc_req);
	if (!val) {
		LOGWARNING("%s:%s Failed to get valid json response to eth_getWork", cs->url, cs->port);
		return ret;
	}
	res_val = json_object_get(val, "result");
	if (!res_val) {
		LOGWARNING("Failed to get result in json response to eth_getWork");
		goto out;
	}

	blockhashmerge = json_string_value(json_object_get(res_val, "blockHashForMergedMining"));
	difficulty = json_string_value(json_object_get(res_val, "difficultyBI"));

	LOGINFO("Rootstock: work: '%s', diff: %s", blockhashmerge, difficulty);

	strcpy(rgw->blockhashmerge, blockhashmerge);
	
	hex2bin(rgw->blockhashmergebin, blockhashmerge, 32);

	hex2bin(diff_swap, difficulty, 32);
	bswap_256(tmp, diff_swap);
	rgw->difficulty = diff_from_target((uchar *)tmp);

	ret = true;
out:
	json_decref(val);
	return ret;
}

static const char* rsk_processSPVProof_req = "{\"jsonrpc\": \"2.0\", \"method\": \"eth_processSPVProof\", \"params\": [\"%s\"], \"id\": %d}\n";

static bool rsk_processSPVProof(connsock_t *cs, char *params)
{
	ckpool_t *ckp = cs->ckp;
	rdata_t *rdata = ckp->rdata;
	json_t *val, *res_val;
	int len, retries = 0;
	const char *res_ret;
	bool ret = false;
	char *rpc_req;
	int id;

	len = 76 + strlen(params) + 16; // len(rsk_processSPVProof_req) + len(params) + len(id)
retry:
	id = ++rdata->lastreqid;
	rpc_req = ckalloc(len);
	sprintf(rpc_req, rsk_processSPVProof_req, params, id);
	val = json_rpc_call(cs, rpc_req);
	dealloc(rpc_req);
	if (!val) {
		LOGWARNING("%s:%s Failed to get valid json response to eth_processSPVProof", cs->url, cs->port);
		if (++retries < 5)
			goto retry;
		return ret;
	}
	res_val = json_object_get(val, "result");
	if (!res_val) {
		LOGWARNING("Failed to get result in json response to eth_processSPVProof");
		if (++retries < 5) {
			json_decref(val);
			goto retry;
		}
		goto out;
	}
	if (!json_is_null(res_val)) {
		res_ret = json_string_value(res_val);
		if (res_ret && strlen(res_ret)) {
			LOGWARNING("SUBMIT BLOCK RETURNED: %s", res_ret);
			/* Consider duplicate response as an accepted block */
			if (safecmp(res_ret, "duplicate"))
				goto out;
		} else {
			LOGWARNING("SUBMIT BLOCK GOT NO RESPONSE!");
			goto out;
		}
	}
	LOGWARNING("eth_processSPVProof ACCEPTED!");
	ret = true;
out:
	json_decref(val);
	return ret;
}

static void *rootstock_update(void *arg)
{
	ckpool_t *ckp = (ckpool_t *)arg;
	rdata_t *rdata = ckp->rdata;
	char *buf = NULL;

	pthread_detach(pthread_self());
	rename_proc("rootstockupdate");

	while (42) {
		dealloc(buf);
		buf = send_recv_proc(ckp->rootstock, "getwork");
		if (buf && strcmp(buf, rdata->lastblockhashmerge) && !cmdmatch(buf, "failed")) {
			LOGWARNING("Rootstock: updating");
			send_proc(ckp->stratifier, "update");
		} else
			cksleep_ms(ckp->rskpollperiod);
	}
	return NULL;
}

/* Use a temporary fd when testing server_alive to avoid races on cs->fd */
static bool server_alive(ckpool_t *ckp, server_instance_t *si, bool pinging)
{
	char *userpass = NULL;
	bool ret = false;
	rdata_t* rdata = ckp->rdata;
	connsock_t *cs;
	rsk_getwork_t *rgw;
	int fd;

	if (si->alive)
		return true;
	cs = &si->cs;
	if (!extract_sockaddr(si->url, &cs->url, &cs->port)) {
		LOGWARNING("Failed to extract address from %s", si->url);
		return ret;
	}
	userpass = strdup(si->auth);
	realloc_strcat(&userpass, ":");
	realloc_strcat(&userpass, si->pass);
	cs->auth = http_base64(userpass);
	dealloc(userpass);
	if (!cs->auth) {
		LOGWARNING("Failed to create base64 auth from %s", userpass);
		return ret;
	}

	fd = connect_socket(cs->url, cs->port);
	if (fd < 0) {
		if (!pinging)
			LOGWARNING("Failed to connect socket to %s:%s !", cs->url, cs->port);
		return ret;
	}

	/* Test we can connect, authorise and get a block template */
	rgw = ckzalloc(sizeof(rsk_getwork_t));
	si->data = rgw;
	if (!rsk_getwork(cs, rgw)) {
		if (!pinging) {
			LOGINFO("Failed to get test block template from %s:%s!",
				cs->url, cs->port);
		}
		goto out;
	} 
	si->alive = ret = true;
	LOGNOTICE("Server alive: %s:%s", cs->url, cs->port);
out:
	/* Close the file handle */
	close(fd);
	return ret;
}

/* Find the highest priority server alive and return it */
static server_instance_t *live_server(ckpool_t *ckp)
{
	server_instance_t *alive = NULL;
	connsock_t *cs;
	int i;

	LOGDEBUG("Attempting to connect to bitcoind");
retry:
	/* First find a server that is already flagged alive if possible
	 * without blocking on server_alive() */
	for (i = 0; i < ckp->rskds; i++) {
		server_instance_t *si = ckp->rskdservers[i];
		cs = &si->cs;

		if (si->alive) {
			alive = si;
			goto living;
		}
	}

	/* No servers flagged alive, try to connect to them blocking */
	for (i = 0; i < ckp->rskds; i++) {
		server_instance_t *si = ckp->rskdservers[i];

		if (server_alive(ckp, si, false)) {
			alive = si;
			goto living;
		}
	}
	LOGWARNING("WARNING: No rskds active!");
	sleep(5);
	goto retry;
living:
	cs = &alive->cs;
	LOGINFO("Connected to rskd server %s:%s", cs->url, cs->port);
	return alive;
}

static void kill_server(server_instance_t *si)
{
	connsock_t *cs;

	if (!si) // This shouldn't happen
		return;

	LOGNOTICE("Killing server");
	cs = &si->cs;
	Close(cs->fd);
	empty_buffer(cs);
	dealloc(cs->url);
	dealloc(cs->port);
	dealloc(cs->auth);
	dealloc(si->data);
}

static void clear_unix_msg(unix_msg_t **umsg)
{
	if (*umsg) {
		Close((*umsg)->sockd);
		free((*umsg)->buf);
		free(*umsg);
		*umsg = NULL;
	}
}

static void rootstock_loop(proc_instance_t *pi)
{
	server_instance_t *si = NULL, *old_si;
	unix_msg_t *umsg = NULL;
	ckpool_t *ckp = pi->ckp;
	rdata_t *rdata = ckp->rdata;
	bool started = false;
	char *buf = NULL;
	connsock_t *cs;
	rsk_getwork_t *rgw;

reconnect:
	clear_unix_msg(&umsg);
	old_si = si;
	si = live_server(ckp);
	if (!si)
		goto out;
	if (unlikely(!started)) {
		started = true;
		LOGWARNING("%s rootstock ready", ckp->name);
	}

	rgw = si->data;
	cs = &si->cs;
	if (!old_si)
		LOGWARNING("Connected to rskd: %s:%s", cs->url, cs->port);
	else if (si != old_si)
		LOGWARNING("Failed over to rskd: %s:%s", cs->url, cs->port);

retry:
	clear_unix_msg(&umsg);

	do {
		umsg = get_unix_msg(pi);
	} while (!umsg);

	if (unlikely(!si->alive)) {
		LOGWARNING("%s:%s Rskd socket invalidated, will attempt failover", cs->url, cs->port);
		goto reconnect;
	}

	buf = umsg->buf;
	LOGDEBUG("Rootstock received request: %s", buf);
	if (cmdmatch(buf, "getwork")) {
		if (!rsk_getwork(cs, rgw)) {
			LOGWARNING("Failed to get work from %s:%s",
				   cs->url, cs->port);
			si->alive = false;
			send_unix_msg(umsg->sockd, "Failed");
			goto reconnect;
		} else {
			memcpy(rdata->blockhashmergebin, rgw->blockhashmergebin, 32);
			rdata->difficulty = rgw->difficulty;
			strcpy(rdata->blockhashmerge, rgw->blockhashmerge);
			send_unix_msg(umsg->sockd, rdata->blockhashmerge);
		}
	} else if (cmdmatch(buf, "submitblock")) {
		bool ret;
		LOGNOTICE("Submitting rootstock block data!");
		ret = rsk_processSPVProof(cs, buf + 12 + 64 + 1);
	} else if (cmdmatch(buf, "reconnect")) {
		goto reconnect;
	} else if (cmdmatch(buf, "loglevel")) {
		sscanf(buf, "loglevel=%d", &ckp->loglevel);
	} else if (cmdmatch(buf, "ping")) {
		LOGDEBUG("Rootstock received ping request");
		send_unix_msg(umsg->sockd, "pong");
	}
	goto retry;

out:
	kill_server(si);
}

/* Check which servers are alive, maintaining a connection with them and
 * reconnect if a higher priority one is available. */
static void *server_watchdog(void *arg)
{
	ckpool_t *ckp = (ckpool_t *)arg;
	rdata_t *rdata = ckp->rdata;

	while (42) {
		server_instance_t *best = NULL;
		ts_t timer_t;
		int i;

		cksleep_prepare_r(&timer_t);
		for (i = 0; i < ckp->rskds; i++) {
			server_instance_t *si = ckp->rskdservers[i];

			/* Have we reached the current server? */
			if (server_alive(ckp, si, true) && !best)
				best = si;
		}
		if (best && best != rdata->si) {
			rdata->si = best;
			send_proc(ckp->rootstock, "reconnect");
		}
		cksleep_ms_r(&timer_t, 5000);
	}
	return NULL;
}

static void setup_servers(ckpool_t *ckp)
{
	pthread_t pth_watchdog;
	int i;

	ckp->rskdservers = ckalloc(sizeof(server_instance_t *) * ckp->rskds);
	for (i = 0; i < ckp->rskds; i++) {
		server_instance_t *si;
		connsock_t *cs;

		ckp->rskdservers[i] = ckzalloc(sizeof(server_instance_t));
		si = ckp->rskdservers[i];
		si->url = ckp->rskdurl[i];
		si->auth = ckp->rskdauth[i];
		si->pass = ckp->rskdpass[i];
		si->id = i;
		cs = &si->cs;
		cs->ckp = ckp;
		cksem_init(&cs->sem);
		cksem_post(&cs->sem);
	}

	create_pthread(&pth_watchdog, server_watchdog, ckp);
}

static void server_mode(ckpool_t *ckp, proc_instance_t *pi)
{
	int i;

	setup_servers(ckp);

	rootstock_loop(pi);

	for (i = 0; i < ckp->rskds; i++) {
		server_instance_t *si = ckp->rskdservers[i];

		kill_server(si);
		dealloc(si);
	}
	dealloc(ckp->servers);
}

void *rootstock(void *arg)
{
	proc_instance_t *pi = (proc_instance_t *)arg;
	ckpool_t *ckp = pi->ckp;
	rdata_t *rdata;

	rename_proc(pi->processname);
	LOGWARNING("%s rootstock starting", ckp->name);
	rdata = ckzalloc(sizeof(rdata_t));
	ckp->rdata = rdata;
	rdata->ckp = ckp;

	server_mode(ckp, pi);
	dealloc(ckp->rdata);
	return NULL;
}
