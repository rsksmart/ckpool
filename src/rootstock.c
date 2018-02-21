/*
 * Copyright 2016
 */

#include "config.h"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <jansson.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "ckpool.h"
#include "rootstock.h"

#include "jansson.h"
#include "hashtable.h"

#include "rsktestconfig.h"

#define BIN_HASH_SIZE sizeof(((rsk_getwork_t*)0)->blockhashmergebin)
#define HEX_HASH_SIZE sizeof(((rsk_getwork_t*)0)->blockhashmerge)
#define HEX_TARGET_SIZE sizeof(((rsk_getwork_t*)0)->target)

static const char *rsk_getwork_req = "{\"jsonrpc\": \"2.0\", \"method\": \"mnr_getWork\", \"params\": [], \"id\": %d}\n";

bool rsk_getwork(connsock_t *cs, rsk_getwork_t *rgw)
{
	ckpool_t *ckp = cs->ckp;
	rdata_t *rdata = ckp->rdata;
	json_t *res_val, *val;
	bool ret = false;
	char *rpc_req;
	const char* blockhashmerge;
	char blockhashmergebin[36];
	const char* target;
	double minerfees = 0.0;
	const char* strminerfees;
	const char* parentblockhash;
	int notify = 0;
	int id;

	id = ++rdata->lastreqid;
	ASPRINTF(&rpc_req, rsk_getwork_req, id);

	val = json_rpc_call_timeout(cs, rpc_req, 3);
	dealloc(rpc_req);
	if (!val) {
		LOGWARNING("%s:%s Failed to get valid json response to mnr_getWork", cs->url, cs->port);
		return ret;
	}
	res_val = json_object_get(val, "result");
	if (!res_val || !json_is_object(res_val)) {
		LOGWARNING("Failed to get result in json response to mnr_getWork");
		goto out;
	}

	if(!json_is_string(json_object_get(res_val, "blockHashForMergedMining")) || !json_is_string(json_object_get(res_val, "target")) ||
			!json_is_string(json_object_get(res_val, "feesPaidToMiner")) || !json_is_boolean(json_object_get(res_val, "notify")) ||
			!json_is_string(json_object_get(res_val, "parentBlockHash"))) {
		LOGWARNING("Failed to get one of the values from result in json response to mnr_getWork");
		goto out;
	}

	blockhashmerge = json_string_value(json_object_get(res_val, "blockHashForMergedMining"));
	target = json_string_value(json_object_get(res_val, "target"));

	strminerfees = json_string_value(json_object_get(res_val, "feesPaidToMiner"));
	minerfees = atof(strminerfees);
	notify = json_is_true(json_object_get(res_val, "notify"));

	parentblockhash = json_string_value(json_object_get(res_val, "parentBlockHash"));

	// + 2 is to skip the initial 0X value of the hex representation of the hash
	strncpy(rgw->blockhashmerge, blockhashmerge + 2, HEX_HASH_SIZE);
	hex2bin(blockhashmergebin, blockhashmerge + 2, BIN_HASH_SIZE);
	memcpy(rgw->blockhashmergebin, blockhashmergebin, BIN_HASH_SIZE);

	strncpy(rgw->target, target + 2, HEX_TARGET_SIZE);

	strncpy(rgw->parentblockhash, parentblockhash+2, HEX_HASH_SIZE);

	rgw->minerfees = minerfees;
	rgw->notify = notify;

	ret = true;
out:
	json_decref(val);
	return ret;
}

static const char* rsk_submitBitcoinSolution_req = "{\"jsonrpc\": \"2.0\", \"method\": \"mnr_submitBitcoinSolution\", \"params\": [\"%s\", \"%s\", \"%s\", \"%s\", \"%s\"], \"id\": %d}\n";

static bool rsk_submitBitcoinSolution(connsock_t *cs, char *blockhash, char *blockheader, char *coinbase, char *merkle_hashes, char* block_txn_count)
{
  ckpool_t *ckp = cs->ckp;
  rdata_t *rdata = ckp->rdata;
  json_t *val, *res_val;
  int retries = 0;
  const char *res_ret;
  bool ret = false;
  char *rpc_req;
  int id;

  retry:
  id = ++rdata->lastreqid;
  ASPRINTF(&rpc_req, rsk_submitBitcoinSolution_req, blockhash, blockheader, coinbase, merkle_hashes, block_txn_count, id);
  val = json_rpc_call_timeout(cs, rpc_req, 3);
  dealloc(rpc_req);

  if (!val) {
    LOGWARNING("%s:%s Failed to get valid json response to mnr_submitBitcoinSolution", cs->url, cs->port);
    if (++retries < 1) {
      Close(cs->fd);
      goto retry;
    }
    return ret;
  }

  res_val = json_object_get(val, "result");
  if (!res_val) {
    res_val = json_object_get(val, "error");
    if(!res_val){
      LOGWARNING("Json response to mnr_submitBitcoinSolution format is unknown and can't be parsed");
      if (++retries < 1) {
        json_decref(val);
        goto retry;
      }
      goto out;
    }
    const int error_code = json_integer_value(json_object_get(res_val, "code"));
    const char *error_message = json_string_value(json_object_get(res_val, "message"));
    LOGWARNING("Error on mnr_submitBitcoinSolution. Code: %d Message: %s.", error_code, error_message);
  }

  if (!json_is_null(res_val)) {
    if(!json_is_string(json_object_get(res_val, "blockImportedResult")) ||
       !json_is_string(json_object_get(res_val, "blockHash")) ||
       !json_is_string(json_object_get(res_val, "blockIncludedHeight"))) {
      LOGWARNING("Failed to get one of the values from result in json response to mnr_submitBitcoinSolution");
      goto out;
    }

    const char *import_result = json_string_value(json_object_get(res_val, "blockImportedResult"));
    const char *hash = json_string_value(json_object_get(res_val, "blockHash"));
    const char *height = json_string_value(json_object_get(res_val, "blockIncludedHeight"));
    LOGWARNING("mnr_submitBitcoinSolution returned. Status: %s Hash: %s Height: %s.", import_result, hash ,height);
    ret = true;
  }
  out:
  json_decref(val);
  return ret;
}

static bool trigger_rsk_update(ckpool_t *ckp, rdata_t* rdata, char *buf)
{
	bool notify_flag_update = (ckp->rsknotifypolicy == 1 || ckp->rsknotifypolicy == 3) && rdata->notify;
	bool different_block_hashUpdate = (ckp->rsknotifypolicy == 2 || ckp->rsknotifypolicy == 4) && strcmp(buf, rdata->lastblockhashmerge);

	return notify_flag_update || different_block_hashUpdate;
}

/*
 * Every "rskpollperiod" invokes on the "main" rskd the getwork command. In some cases (Depending on the getwork result
 * and the pool configuration) notifies the miners
 */
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
		if (buf && !cmdmatch(buf, "failed")) {
			if (trigger_rsk_update(ckp, rdata, buf)) {
				//LOGWARNING("Rootstock: update %s", buf);
				send_proc(ckp->stratifier, "rskupdate");
			}
		}
		cksleep_ms(ckp->rskpollperiod);
	}
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
	if (!cs->auth) {
		LOGWARNING("Failed to create base64 auth from %s", userpass);
		dealloc(userpass);
		return ret;
	}
	dealloc(userpass);

	fd = connect_socket(cs->url, cs->port);
	if (fd < 0) {
		if (!pinging)
			LOGWARNING("Failed to connect socket to %s:%s !", cs->url, cs->port);
		return ret;
	}

	/* Test we can connect, authorise and get a block template */
	rgw = ckzalloc(sizeof(rsk_getwork_t));
	if (!rgw) {
		LOGWARNING("Couldn't allocate an rsk_getwork_t instance");
		goto out;
	}

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
	Close(fd);
	return ret;
}

/* Find the highest priority server alive and return it */
static server_instance_t *live_server(ckpool_t *ckp)
{
	server_instance_t *alive = NULL;
	connsock_t *cs;
	int i;

	LOGDEBUG("Attempting to connect to rskd");
	int attempts = 3;
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

	if (attempts--) {
		LOGWARNING("No rskds active, retry %d more times", attempts);
		goto retry;
	} else {
		LOGWARNING("No rskds active, no more retries");
		goto failed;
	}
living:
	cs = &alive->cs;
	LOGINFO("Connected to rskd server %s:%s", cs->url, cs->port);
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
 * Iterates forever looking for task reading a buffer of messages. If we get a "getWork" or "submitblock" msg
 * we send that to the "main" rskd
 */
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
	if (!si) {
		LOGWARNING("Could not connect to rskd, empty queue");
		do {
			LOGWARNING("Empty 1 rsk queue item");
			clear_unix_msg(&umsg);
			umsg = get_unix_msg_no_wait(pi);
		} while (umsg);
		LOGWARNING("Finished emptying rskd queue");
		goto reconnect;
	}
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
	if (cmdmatch(buf, "getwork")) {
		if (!rsk_getwork(cs, rgw)) {
			LOGWARNING("Failed to get work from %s:%s", cs->url, cs->port);
			si->alive = false;
			send_unix_msg(umsg->sockd, "failed");
			goto reconnect;
		} else {
			memcpy(rdata->blockhashmergebin, rgw->blockhashmergebin, 32);
			strcpy(rdata->target, rgw->target);
			rdata->minerfees = rgw->minerfees;
			rdata->notify = rgw->notify;
			strcpy(rdata->blockhashmerge, rgw->blockhashmerge);
			strcpy(rdata->parentblockhash, rgw->parentblockhash);
			send_unix_msg(umsg->sockd, rdata->blockhashmerge);
		}
    } else if (cmdmatch(buf, "submitBitcoinSolution")) {
      bool ret;
      tv_t start_tv;
      tv_t finish_tv;
      const size_t submit_bitcoin_solution_tag_size = 22;
      const size_t blockhash_size = 64;
      const char delimiter[2] = ",";
      char *submission_data[5];
      int i = 0;

      // Message is not complete so do not even try to parse it
      if(strlen(buf + submit_bitcoin_solution_tag_size + 1) == 0) {
        goto retry;
      }

      // Parse message
      submission_data[0] = strtok(buf + submit_bitcoin_solution_tag_size + 1, delimiter);
      while(submission_data[i] != NULL) {
        i++;
        submission_data[i] = strtok(NULL, delimiter);
      }

      // Every block must have hash, header, coinbase tx, merkle hashes and txn_count
      if(!submission_data[0] || !submission_data[1] || !submission_data[2] || !submission_data[3] || !submission_data[4]) {
        LOGWARNING("blockheader and/or coinbase is empty");
        goto retry;
      }

      LOGINFO("Submitting rootstock solution data!");

      tv_time(&start_tv);
      ret = rsk_submitBitcoinSolution(cs, submission_data[0], submission_data[1], submission_data[2], submission_data[3], submission_data[4]);
      tv_time(&finish_tv);

      {
        struct tm start_tm;
        int start_ms = (int)(start_tv.tv_usec / 1000);
        struct tm finish_tm;
        int finish_ms = (int)(finish_tv.tv_usec / 1000);
        localtime_r(&(start_tv.tv_sec), &start_tm);
        localtime_r(&(finish_tv.tv_sec), &finish_tm);
        memset(buf + submit_bitcoin_solution_tag_size + blockhash_size, 0, 1);
        LOGINFO("ROOTSTOCK: submitBitcoinSolution: %d-%02d-%02d %02d:%02d:%02d.%03d, %d-%02d-%02d %02d:%02d:%02d.%03d, %s",
                start_tm.tm_year + 1900, start_tm.tm_mon + 1, start_tm.tm_mday,
                start_tm.tm_hour, start_tm.tm_min, start_tm.tm_sec, start_ms,
                finish_tm.tm_year + 1900, finish_tm.tm_mon + 1, finish_tm.tm_mday,
                finish_tm.tm_hour, finish_tm.tm_min, finish_tm.tm_sec, finish_ms,
                buf + 12);
      }
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
	pthread_t pth_rskupdate;
	int i;

	if (ckp->rskds > INT_MAX/sizeof(server_instance_t *)) {
		LOGWARNING("Too many rskd servers to connect. First two will be used instead.");
		ckp->rskds = 2;
	}

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
	create_pthread(&pth_rskupdate, rootstock_update, ckp);
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
}
