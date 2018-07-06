/*
 * Copyright 2018
 */

#include "ckpool.h"
#include "emercoin.h"

#include <limits.h>

static const char *emc_getauxblock_req = "{\"jsonrpc\": \"2.0\", \"method\": \"getauxblock\", \"params\": [], \"id\": %d}\n";

bool emc_getauxblock(connsock_t *cs, emc_auxblock_t *auxblock)
{
    ckpool_t *ckp = cs->ckp;
	emcdata_t *emcdata = ckp->emcdata;
	json_t *res_val, *val;
	bool ret = false;
	char *rpc_req;
	const char* hashmerge;
	const char* target;
	char hashmergebin[BIN_HASH_SIZE];
	int chainid = 0;
	int id;

	id = ++emcdata->lastreqid;
	ASPRINTF(&rpc_req, emc_getauxblock_req, id);

	val = json_rpc_call_timeout(cs, rpc_req, 3);
	dealloc(rpc_req);
	if (!val) {
		LOGWARNING("%s:%s Failed to get valid json response to getauxblock", cs->url, cs->port);
		return ret;
	}

	res_val = json_object_get(val, "result");
	if (!res_val || !json_is_object(res_val)) {
		LOGWARNING("Failed to get result in json response to getauxblock");
		goto out;
	}
    	
	if (!json_is_string(json_object_get(res_val, "target")) || 
        !json_is_string(json_object_get(res_val, "hash")) ||
		!json_is_integer(json_object_get(res_val, "chainid"))) {
		LOGWARNING("Failed to get one of the values from result in json response to getauxblock");
		goto out;
	}

    // value has no 0x preceding it
	hashmerge = json_string_value(json_object_get(res_val, "hash"));
	target = json_string_value(json_object_get(res_val, "target"));

	chainid = json_integer_value(json_object_get(res_val, "chainid"));
	
	strncpy(auxblock->hashmerge, hashmerge, STR_HEX_HASH_SIZE);
	hex2bin(hashmergebin, hashmerge, BIN_HASH_SIZE);
	memcpy(auxblock->hashmergebin, hashmergebin, BIN_HASH_SIZE);

	strncpy(auxblock->target, target, STR_HEX_HASH_SIZE);

    auxblock->chainid = chainid;

	ret = true;
out:
	json_decref(val);
	return ret;
}


static const char* emc_submitauxblock_req = "{\"jsonrpc\": \"2.0\", \"method\": \"getauxblock\", \"params\": [\"%s\", \"%s\"], \"id\": %d}\n";

static void emc_submitauxblock(connsock_t *cs, char *blockhex)
{
    ckpool_t *ckp = cs->ckp;
    emcdata_t *emcdata = ckp->emcdata;
    json_t *res_val, *val;
    char *rpc_req;
    int retries = 0;
    int id;

retry:
    id = ++emcdata->lastreqid;
    ASPRINTF(&rpc_req, emc_submitauxblock_req, emcdata->hashmerge, blockhex, id);
    val = json_rpc_call_timeout(cs, rpc_req, 3);
    dealloc(rpc_req);

    if (!val) {
        LOGWARNING("%s:%s Failed to get valid json response to emercoin getauxblock submission", cs->url, cs->port);
        if (++retries < 1) {
            Close(cs->fd);
            goto retry;
        }
        goto out;
    } 

    // parse result
    res_val = json_object_get(val, "result");
    if (!res_val) {
        // result not valid, check for error
        res_val = json_object_get(val, "error");
        if (!res_val) {
            LOGWARNING("Json response to emc getauxblock submission format is unknown and can't be parsed");
            if (++retries < 1) {
                json_decref(val);
                goto retry;
            }
            goto out;
        }
        const int error_code = json_integer_value(json_object_get(res_val, "code"));
        const char *error_message = json_string_value(json_object_get(res_val, "message"));
        LOGWARNING("Error on emc getauxblock submission. Code: %d Message: %s.", error_code, error_message);
        goto out;
    }
	LOGWARNING("Emercoin block accepted!");
out:
    json_decref(val);
    
}

static bool trigger_emc_update(ckpool_t *ckp, emcdata_t* emcdata, char *buf)
{
    return strcmp(buf, emcdata->lasthashmerge) != 0;
}

/* Use a temporary fd when testing server_alive to avoid races on cs->fd */
static bool server_alive(ckpool_t *ckp, server_instance_t *si, bool pinging)
{
	char *userpass = NULL;
	bool ret = false;
	emcdata_t* emcdata = ckp->emcdata;
	connsock_t *cs;
	emc_auxblock_t *auxblock;
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
	if (!cs->auth) {
		LOGWARNING("Failed to create base64 auth from provided user and password");
		dealloc(userpass);
		return ret;
	}
	dealloc(userpass);

	fd = connect_socket(cs->url, cs->port);
	if (fd < 0) {
		if (!pinging)
			LOGWARNING("Failed to connect socket to %s:%s!", cs->url, cs->port);
		return ret;
	}

	/* Test we can connect, authorise and get a block template */
	auxblock = ckzalloc(sizeof(emc_auxblock_t));
	if (!auxblock) {
		LOGWARNING("Couldn't allocate an emc_getauxblock_t instance");
		goto out;
	}

	si->data = auxblock;
	if (!emc_getauxblock(cs, auxblock)) {
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
	Close(fd);
	return ret;
}

/*
 * Every "emcpollperiod" invokes on the "main" emcd the getwork command. In some cases (Depending on the getwork result
 * and the pool configuration) notifies the miners
 */
static void *emercoin_update(void *arg)
{
	server_instance_t *si = NULL;
	ckpool_t *ckp = (ckpool_t *)arg;
	emcdata_t *emcdata = ckp->emcdata;
	char *buf = NULL;
	bool alive = false;
	int i;

	pthread_detach(pthread_self());
	rename_proc("emercoinupdate");

	while (42) {
		dealloc(buf);

		// check if any of emcd is alive. if not, do not even try to ask for work.
		alive = false;
		for (i = 0; i < ckp->emcds; i++) {
			server_instance_t *si = ckp->emcdservers[i];
			if (server_alive(ckp, si, false)) {
				alive = true;
				break;
			}
		}

		if (alive) {
			buf = send_recv_proc(ckp->emercoin, "getauxblock");
			if (buf && !cmdmatch(buf, "failed")) {
				if (trigger_emc_update(ckp, emcdata, buf)) {
					send_proc(ckp->stratifier, "emcupdate");
				}
			}
		}

		cksleep_ms(ckp->emcpollperiod);
	}
}

/* Find the highest priority server alive and return it */
static server_instance_t *live_server(ckpool_t *ckp)
{
	server_instance_t *alive = NULL;
	connsock_t *cs;
	int i;

	LOGDEBUG("Attempting to connect to emcd");
	int attempts = 3;
retry:
	/* First find a server that is already flagged alive if possible
	 * without blocking on server_alive() */
	for (i = 0; i < ckp->emcds; i++) {
		server_instance_t *si = ckp->emcdservers[i];
		cs = &si->cs;

		if (si->alive) {
			alive = si;
			goto living;
		}
	}

	/* No servers flagged alive, try to connect to them blocking */
	for (i = 0; i < ckp->emcds; i++) {
		server_instance_t *si = ckp->emcdservers[i];

		if (server_alive(ckp, si, false)) {
			alive = si;
			goto living;
		}
	}
	LOGWARNING("WARNING: No emcds active!");
	sleep(5);

	if (attempts--) {
		LOGWARNING("No emcds active, retry %d more times", attempts);
		goto retry;
	} else {
		LOGWARNING("No emcds active, no more retries");
		goto failed;
	}
living:
	cs = &alive->cs;
	LOGINFO("Connected to emcd server %s:%s", cs->url, cs->port);
	return alive;
failed:
	return NULL;
}

static void kill_server(server_instance_t *si)
{
	connsock_t *cs;

	if (unlikely(!si)) // This shouldn't happen
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

/**
 * Iterates forever looking for task reading a buffer of messages. 
 * If we get a "getauxblock" or "submitauxblock" msg we send that to the "main" emcd
 */
static void emercoin_loop(proc_instance_t *pi)
{
	server_instance_t *si = NULL, *old_si;
	unix_msg_t *umsg = NULL;
	ckpool_t *ckp = pi->ckp;
	emcdata_t *emcdata = ckp->emcdata;
	bool started = false;
	char *buf = NULL;
	connsock_t *cs;
	emc_auxblock_t *auxblock;

reconnect:
	clear_unix_msg(&umsg);
	old_si = si;
	si = live_server(ckp);
	if (!si) {
		LOGWARNING("Could not connect to emcd, empty queue");
		do {
			LOGWARNING("Empty 1 emc queue item");
			clear_unix_msg(&umsg);
			umsg = get_unix_msg_no_wait(pi);
		} while (umsg);
		LOGWARNING("Finished emptying emcd queue");
		goto reconnect;
	}
	if (unlikely(!started)) {
		started = true;
		LOGWARNING("%s emercoin ready", ckp->name);
	}

	auxblock = si->data;

	cs = &si->cs;
	if (!old_si)
		LOGWARNING("Connected to emcd: %s:%s", cs->url, cs->port);
	else if (si != old_si)
		LOGWARNING("Failed over to emcd: %s:%s", cs->url, cs->port);

retry:
	clear_unix_msg(&umsg);

	do {
		umsg = get_unix_msg(pi);
	} while (!umsg);

	if (unlikely(!si->alive)) {
		LOGWARNING("%s:%s Emcd socket invalidated, will attempt failover", cs->url, cs->port);
		goto reconnect;
	}

	buf = umsg->buf;
	if (cmdmatch(buf, "getauxblock")) {
		if (!emc_getauxblock(cs, auxblock)) {
			LOGWARNING("Failed to get aux block from %s:%s", cs->url, cs->port);
			si->alive = false;
			send_unix_msg(umsg->sockd, "failed");
			goto reconnect;
		} else {
			memcpy(emcdata->hashmergebin, auxblock->hashmergebin, sizeof(emcdata->hashmergebin));
			strncpy(emcdata->hashmerge, auxblock->hashmerge, sizeof(emcdata->hashmerge));
			strncpy(emcdata->target, auxblock->target, sizeof(emcdata->target));
			emcdata->chainid = auxblock->chainid;

			send_unix_msg(umsg->sockd, emcdata->hashmerge);
		}
    } else if (cmdmatch(buf, "emcsubmitauxblock")) {
      
        const size_t tag_size = 18; // size of "emcsubmitauxblock:"
        const char *blockhex = buf + tag_size;

        tv_t start_tv;
        tv_t finish_tv;

        LOGINFO("Submitting emercoin solution data");

        tv_time(&start_tv);
        emc_submitauxblock(cs, blockhex);
        tv_time(&finish_tv);

        {
            struct tm start_tm;
            int start_ms = (int)(start_tv.tv_usec / 1000);
            struct tm finish_tm;
            int finish_ms = (int)(finish_tv.tv_usec / 1000);
            localtime_r(&(start_tv.tv_sec), &start_tm);
            localtime_r(&(finish_tv.tv_sec), &finish_tm);
            //memset(buf + submit_bitcoin_solution_tag_size + HASH_SIZE * 2, 0, 1);
            LOGINFO("EMERCOIN: emcsubmitauxblock: %d-%02d-%02d %02d:%02d:%02d.%03d, %d-%02d-%02d %02d:%02d:%02d.%03d, %s",
                    start_tm.tm_year + 1900, start_tm.tm_mon + 1, start_tm.tm_mday,
                    start_tm.tm_hour, start_tm.tm_min, start_tm.tm_sec, start_ms,
                    finish_tm.tm_year + 1900, finish_tm.tm_mon + 1, finish_tm.tm_mday,
                    finish_tm.tm_hour, finish_tm.tm_min, finish_tm.tm_sec, finish_ms,
                    blockhex);
        }
	} else if (cmdmatch(buf, "reconnect")) {
		goto reconnect;
	} else if (cmdmatch(buf, "loglevel")) {
		sscanf(buf, "loglevel=%d", &ckp->loglevel);
	} else if (cmdmatch(buf, "ping")) {
		LOGDEBUG("Emercoin received ping request");
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
	emcdata_t *emcdata = ckp->emcdata;

	while (42) {
		server_instance_t *best = NULL;
		ts_t timer_t;
		int i;

		cksleep_prepare_r(&timer_t);
		for (i = 0; i < ckp->emcds; i++) {
			server_instance_t *si = ckp->emcdservers[i];

			/* Have we reached the current server? */
			if (server_alive(ckp, si, true) && !best)
				best = si;
		}
		if (best && best != emcdata->si) {
			emcdata->si = best;
			send_proc(ckp->emercoin, "reconnect");
		}
		cksleep_ms_r(&timer_t, 5000);
	}
	return NULL;
}

static void setup_servers(ckpool_t *ckp)
{
	pthread_t pth_watchdog;
	pthread_t pth_emcupdate;
	int i;

	if (ckp->emcds > INT_MAX/sizeof(server_instance_t *)) {
		LOGWARNING("Too many emcd servers to connect. First two will be used instead.");
		ckp->emcds = 2;
	}

	ckp->emcdservers = ckalloc(sizeof(server_instance_t *) * ckp->emcds);
	for (i = 0; i < ckp->emcds; i++) {
		server_instance_t *si;
		connsock_t *cs;

		ckp->emcdservers[i] = ckzalloc(sizeof(server_instance_t));
		si = ckp->emcdservers[i];
		si->url = ckp->emcdurl[i];
		si->auth = ckp->emcdauth[i];
		si->pass = ckp->emcdpass[i];
		si->id = i;
		cs = &si->cs;
		cs->ckp = ckp;
		cksem_init(&cs->sem);
		cksem_post(&cs->sem);
	}

	create_pthread(&pth_watchdog, server_watchdog, ckp);
	create_pthread(&pth_emcupdate, emercoin_update, ckp);
}

static void server_mode(ckpool_t *ckp, proc_instance_t *pi) {
	int i;

	setup_servers(ckp);

	emercoin_loop(pi);

	for (i = 0; i < ckp->emcds; i++) {
		server_instance_t *si = ckp->emcdservers[i];

		kill_server(si);
		dealloc(si);
	}
	dealloc(ckp->servers);
}

void* emercoin(void* arg) {
	proc_instance_t *pi = (proc_instance_t *)arg;
	ckpool_t *ckp = pi->ckp;
    emcdata_t *emcdata;

	rename_proc(pi->processname);
	LOGWARNING("%s emercoin starting", ckp->name);
	emcdata = ckzalloc(sizeof(emcdata_t));
	ckp->emcdata = emcdata;
	emcdata->ckp = ckp;

	server_mode(ckp, pi);
	dealloc(ckp->emcdata);
}